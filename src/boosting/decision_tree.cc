/* decision_tree.cc                                                -*- C++ -*-
   Jeremy Barnes, 22 March 2004
   Copyright (c) 2004 Jeremy Barnes.  All rights reserved.
   $Source$

   Implementation of the decision tree.
*/

#include "decision_tree.h"
#include "classifier_persist_impl.h"
#include <boost/progress.hpp>
#include <boost/timer.hpp>
#include <functional>
#include "utils/vector_utils.h"
#include "config_impl.h"


using namespace std;
using namespace DB;



namespace ML {


/*****************************************************************************/
/* DECISION_TREE                                                             */
/*****************************************************************************/

Decision_Tree::Decision_Tree()
    : encoding(OE_PROB)
{
}

Decision_Tree::
Decision_Tree(DB::Store_Reader & store,
              const boost::shared_ptr<const Feature_Space> & fs)
{
    throw Exception("Decision_Tree constructor(reconst): not implemented");
}
    
Decision_Tree::
Decision_Tree(boost::shared_ptr<const Feature_Space> feature_space,
              const Feature & predicted)
    : Classifier_Impl(feature_space, predicted)
{
}
    
Decision_Tree::~Decision_Tree()
{
}
    
void Decision_Tree::swap(Decision_Tree & other)
{
    Classifier_Impl::swap(other);
    std::swap(tree, other.tree);
    std::swap(encoding, other.encoding);
}

float Decision_Tree::
predict(int label, const Feature_Set & features) const
{
    /* Simple version, for now. */
    return predict(features).at(label);
}

distribution<float>
Decision_Tree::
predict(const Feature_Set & features) const
{
    try {
        const distribution<float> & result
            = predict_recursive(features, tree.root);
        //cerr << "Decision_Tree::predict(): returning " << result << endl;
        return result;
    } catch (const std::exception & exc) {
        cerr << "tree: " << print() << endl;
        cerr << "features: " << feature_space()->print(features)
             << endl;
        throw;
    }
}

distribution<float>
Decision_Tree::
predict_recursive(const Feature_Set & features,
                  const Tree::Ptr & ptr) const
{
    if (!ptr) return distribution<float>(label_count(), 0.0);
    else if (!ptr.node()) return ptr.leaf()->pred;

    const Tree::Node & node = *ptr.node();
    
    Split::Weights weights = node.split.apply(features);
    
    /* Go down all of the edges that we need to for this example. */
    distribution<float> result(label_count(), 0.0);
    if (weights[true] > 0.0)
        result += weights[true] * predict_recursive(features, node.child_true);
    if (weights[false] > 0.0)
        result += weights[false] * predict_recursive(features, node.child_false);
    if (weights[MISSING] > 0.0)
        result += weights[MISSING] * predict_recursive(features, node.child_missing);
    return result;
}

string Decision_Tree::
print_recursive(int level, const Tree::Ptr & ptr,
                float total_weight) const
{
    string spaces(level * 4, ' ');
    if (ptr.node()) {
        Tree::Node & n = *ptr.node();
        string result;
        float cov = n.examples / total_weight;
        float z_adj = n.z / cov;
        result += spaces 
            + format(" %s (z = %.4f, weight = %.2f, cov = %.2f%%)\n",
                     n.split.print(*feature_space()).c_str(),
                     z_adj, n.examples, cov * 100.0);
        result += spaces + "  true: \n";
        result += print_recursive(level + 1, n.child_true, total_weight);
        result += spaces + "  false: \n";
        result += print_recursive(level + 1, n.child_false, total_weight);
        result += spaces + "  missing: \n";
        result += print_recursive(level + 1, n.child_missing, total_weight);
        return result;
    }
    else if (ptr.leaf()) {
        string result = spaces + "leaf: ";
        Tree::Leaf & l = *ptr.leaf();
        const distribution<float> & dist = l.pred;
        for (unsigned i = 0;  i < dist.size();  ++i)
            if (dist[i] != 0.0) result += format(" %d/%.3f", i, dist[i]);
        float cov = l.examples / total_weight;
        result += format(" (weight = %.2f, cov = %.2f%%)\n",
                         l.examples, cov * 100.0);
        
        return result + "\n";
    }
    else return spaces + "NULL";
}

std::string Decision_Tree::print() const
{
    string result = "Decision tree:\n";
    float total_weight = 0.0;
    if (tree.root.node()) total_weight = tree.root.node()->examples;
    result += print_recursive(0, tree.root, total_weight);
    return result;
}

std::string Decision_Tree::summary() const
{
    if (!tree.root)
        return "NULL";
    
    float total_weight = 0.0;
    if (tree.root.node()) total_weight = tree.root.node()->examples;
    
    if (tree.root.node()) {
        Tree::Node & n = *tree.root.node();
        float cov = n.examples / total_weight;
        float z_adj = n.z / cov;
        return "Root: " + n.split.print(*feature_space())
            + format(" (z = %.4f)", z_adj);
    }
    else {
        string result = "leaf: ";
        Tree::Leaf & l = *tree.root.leaf();
        const distribution<float> & dist = l.pred;
        for (unsigned i = 0;  i < dist.size();  ++i)
            if (dist[i] != 0.0) result += format(" %d/%.3f", i, dist[i]);
        return result;
    }
}

namespace {

void all_features_recursive(const Tree::Ptr & ptr,
                            vector<Feature> & result)
{
    if (ptr.node()) {
        Tree::Node & n = *ptr.node();
        result.push_back(n.split.feature());

        all_features_recursive(n.child_true,    result);
        all_features_recursive(n.child_false,   result);
        all_features_recursive(n.child_missing, result);
    }
}

} // file scope

std::vector<ML::Feature> Decision_Tree::all_features() const
{
    std::vector<ML::Feature> result;
    all_features_recursive(tree.root, result);
    make_vector_set(result);
    return result;
}

Output_Encoding
Decision_Tree::
output_encoding() const
{
    return encoding;
}

void Decision_Tree::serialize(DB::Store_Writer & store) const
{
    store << string("DECISION_TREE");
    store << compact_size_t(3);  // version
    store << compact_size_t(label_count());
    feature_space_->serialize(store, predicted_);
    tree.serialize(store, *feature_space());
    store << encoding;
    store << compact_size_t(12345);  // end marker
}

void Decision_Tree::
reconstitute(DB::Store_Reader & store,
             const boost::shared_ptr<const Feature_Space> & feature_space)
{
    string id;
    store >> id;

    if (id != "DECISION_TREE")
        throw Exception("Decision_Tree::reconstitute: read bad ID '"
                        + id + "'");

    compact_size_t version(store);
    
    switch (version) {
    case 1: {
        compact_size_t label_count(store);
        Classifier_Impl::init(feature_space, MISSING_FEATURE, label_count);
        tree.reconstitute(store, *feature_space);
        break;
    }
    case 2:
    case 3: {
        compact_size_t label_count(store);
        feature_space->reconstitute(store, predicted_);
        Classifier_Impl::init(feature_space, predicted_);
        tree.reconstitute(store, *feature_space);
        if (version >= 3)
            store >> encoding;
        else encoding = OE_PROB;
        break;
    }
    default:
        throw Exception("Decision tree: Attempt to reconstitute tree of "
                        "unknown version " + ostream_format(version.size_));
    }

    compact_size_t marker(store);
    if (marker != 12345)
        throw Exception("Decision_Tree::reconstitute: read bad marker at end");
}
    
std::string Decision_Tree::class_id() const
{
    return "DECISION_TREE";
}

Decision_Tree * Decision_Tree::make_copy() const
{
    return new Decision_Tree(*this);
}

/*****************************************************************************/
/* REGISTRATION                                                              */
/*****************************************************************************/

namespace {

Register_Factory<Classifier_Impl, Decision_Tree> REGISTER("DECISION_TREE");

} // file scope

} // namespace ML
