/* bagging_generator.cc
   Jeremy Barnes, 15 March 2006
   Copyright (c) 2006 Jeremy Barnes  All rights reserved.
   $Source$

   Generator for boosted stumps.
*/

#include "bagging_generator.h"
#include "registry.h"
#include <boost/timer.hpp>
#include <boost/progress.hpp>
#include "training_index.h"
#include "weighted_training.h"
#include "committee.h"
#include "utils/sgi_numeric.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include "worker_task.h"
#include "utils/guard.h"
#include <boost/bind.hpp>
#include "utils/smart_ptr_utils.h"


using namespace std;


namespace ML {

/*****************************************************************************/
/* BAGGING_GENERATOR                                                         */
/*****************************************************************************/

Bagging_Generator::
Bagging_Generator()
{
    defaults();
}

Bagging_Generator::~Bagging_Generator()
{
}

void
Bagging_Generator::
configure(const Configuration & config)
{
    Early_Stopping_Generator::configure(config);
    
    config.find(num_bags,         "num_bags");
    config.find(validation_split, "validation_split");
    config.find(testing_split,    "testing_split");

    weak_learner = get_trainer("weak_learner", config);
}

void
Bagging_Generator::
defaults()
{
    Early_Stopping_Generator::defaults();
    num_bags = 10;
    validation_split = 0.35;
    testing_split = 0.0;
    weak_learner.reset();
}

Config_Options
Bagging_Generator::
options() const
{
    Config_Options result = Early_Stopping_Generator::options();
    result
        .add("num_bags", num_bags, "N>=1",
             "number of bags to divide classifier into")
        .add("validation_split", validation_split, "0<N<=1",
             "how much of training data to hold off as validation data")
        .add("testing_split", testing_split, "0<N<=1",
             "how much of training data to hold off as testing data (optional)")
        .subconfig("weak_leaner", weak_learner,
                   "weak learner that produces each bag");
    
    return result;
}

void
Bagging_Generator::
init(boost::shared_ptr<const Feature_Space> fs, Feature predicted)
{
    Classifier_Generator::init(fs, predicted);
    weak_learner->init(fs, predicted);
}

namespace {

template<class RNG>
struct RNG_Adaptor {

    RNG_Adaptor(RNG & rng)
        : rng(rng)
    {
    }

    RNG & rng;
    
    template<class T>
    T operator () (T val) const
    {
        return rng() % val;
    }
};

struct Bag_Job_Info {
    Thread_Context & context;
    const Training_Data & training_set;
    const distribution<float> & training_ex_weights;
    const vector<Feature> & features;
    vector<boost::shared_ptr<Classifier_Impl> > & results;
    vector<uint32_t> & random_seeds;
    float train_prop;
    boost::shared_ptr<Classifier_Generator> weak_learner;
    boost::progress_display * progress;
    
    Bag_Job_Info(Thread_Context & context,
                 const Training_Data & training_set,
                 const distribution<float> & training_ex_weights,
                 const vector<Feature> & features,
                 vector<boost::shared_ptr<Classifier_Impl> > & results,
                 vector<uint32_t> & random_seeds,
                 float train_prop,
                 boost::shared_ptr<Classifier_Generator> weak_learner)
        : context(context),
          training_set(training_set), training_ex_weights(training_ex_weights),
          features(features), results(results), random_seeds(random_seeds),
          train_prop(train_prop), weak_learner(weak_learner),
          progress(0)
    {
    }
};

struct Bag_Job {
    Bag_Job(Bag_Job_Info & info, int bag_num, int num_bags, int verbosity)
        : info(info), bag_num(bag_num), num_bags(num_bags),
          verbosity(verbosity)
    {
    }

    Bag_Job_Info & info;
    int bag_num;
    int num_bags;
    int verbosity;

    typedef boost::mt19937 engine_type;

    void operator () () const
    {
        engine_type engine(info.random_seeds[bag_num]);
        RNG_Adaptor<engine_type> rng(engine);

        int nx = info.training_set.example_count();
        /* Partition the dataset. */

#if 0    
        distribution<float> test_eq_weights, test_uniform_weights;
        
        if (test) {
            test_eq_weights = apply_weight_spec(*test, weight_spec);
            test_uniform_weights = distribution<float>(test->example_count(), 1.0);
        }
#endif
        
        distribution<float> in_training(nx);
        vector<int> tr_ex_nums(nx);
        std::iota(tr_ex_nums.begin(), tr_ex_nums.end(), 0);
        std::random_shuffle(tr_ex_nums.begin(), tr_ex_nums.end(), rng);
        for (unsigned i = 0;  i < nx * info.train_prop;  ++i)
            in_training[tr_ex_nums[i]] = 1.0;
        distribution<float> not_training(nx, 1.0);
        not_training -= in_training;

        distribution<float> example_weights(nx);
        
        /* Generate our example weights. */
        for (unsigned i = 0;  i < nx;  ++i)
            example_weights[rng(nx)] += 1.0;

        distribution<float> training_weights
            = in_training * example_weights * info.training_ex_weights;
        training_weights.normalize();

        distribution<float> validate_weights
            = not_training * example_weights * info.training_ex_weights;
        validate_weights.normalize();

        if (verbosity > 0)
            cerr << "bag " << bag_num << " of " << num_bags << endl;

#if 0
        cerr << "train_prop = " << info.train_prop << endl;
        cerr << "in_training = " << in_training << endl;
        cerr << "example_weights = " << example_weights << endl;
        cerr << "info.training_ex_weights = " << info.training_ex_weights
             << endl;
        cerr << "training_weights = " << training_weights << endl;
        cerr << "validate_weights = " << validate_weights << endl;
#endif

        /* Train me! */
        boost::shared_ptr<Classifier_Impl> bag
            = info.weak_learner
            ->generate(info.context, info.training_set, info.training_set,
                       training_weights, validate_weights,
                       info.features);

        /* No need to lock since we're the only one accessing this part of
           the array. */
        info.results[bag_num] = bag;

        if (info.progress)
            ++(*info.progress);

#if 0
        if (test) {
            cerr << "testing results: "
                 << bag->accuracy(*test, test_eq_weights) * 100.0
                 << "% eq this bag, "
                 << result.accuracy(*test, test_eq_weights) * 100.0
                 << "% eq overall, "
                 << result.accuracy(*test, test_uniform_weights) * 100
                 << "% uniform overall" << endl;
        }
#endif
        //cerr << endl;
    }
};

} // file scope


boost::shared_ptr<Classifier_Impl>
Bagging_Generator::
generate(Thread_Context & context,
         const Training_Data & training_set,
         const Training_Data & validation_set,
         const distribution<float> & training_ex_weights,
         const distribution<float> & validate_ex_weights,
         const std::vector<Feature> & features, int) const
{
    boost::timer timer;

    float train_prop = 1.0 - validation_split - testing_split;

    if (train_prop <= 0.0 || train_prop > 1.00001)
        throw Exception("Training proportion out of range");

    if (validation_split < 0.0 || validation_split > 1.00001)
        throw Exception("Validation proportion out of range");

    if (testing_split < 0.0 || testing_split > 1.00001)
        throw Exception("Testing proportion out of range");

    //cerr << "train_prop = " << train_prop << " validation_split = "
    //     << validation_split << " testing_split = " << testing_split
    //     << endl;

    vector<boost::shared_ptr<Classifier_Impl> > results(num_bags);
    vector<uint32_t> random_seeds(num_bags);
    for (unsigned i = 0;  i < num_bags;  ++i)
        random_seeds[i] = context.random();
    
    Bag_Job_Info info(context, training_set, training_ex_weights,
                      features, results,
                      random_seeds, train_prop, weak_learner);
    static Worker_Task & worker = Worker_Task::instance(num_threads() - 1);
    
    int group;
    {
        group = worker.get_group(NO_JOB,
                                 format("Bagging_Generator::generate(): under %d",
                                        context.group()),
                                 context.group());
        //cerr << "bagging: group = " << group << endl;
        Call_Guard guard(boost::bind(&Worker_Task::unlock_group,
                                     boost::ref(worker),
                                     group));
        for (unsigned i = 0;  i < num_bags;  ++i)
            worker.add(Bag_Job(info, i, num_bags, verbosity),
                       format("Bagging_Generator::generate() bag %d under %d",
                              i, group),
                       group);
    }
    
    worker.run_until_finished(group);
    
    Committee result(feature_space, predicted);
    
    for (unsigned i = 0;  i < num_bags;  ++i)
        result.add(results[i], 1.0 / num_bags);
    
    if (profile)
        cerr << "training time: " << timer.elapsed() << "s" << endl;
    
    return make_sp(result.make_copy());
}


/*****************************************************************************/
/* REGISTRATION                                                              */
/*****************************************************************************/

namespace {

Register_Factory<Classifier_Generator, Bagging_Generator>
    BAGGING_REGISTER("bagging");

} // file scope

} // namespace ML