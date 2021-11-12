#pragma once

#include <torch/csrc/lazy/core/ir.h>
#include <torch/csrc/lazy/core/shape.h>

#include "c10/util/ArrayRef.h"
#include "lazy_tensor_core/csrc/lowering_context.h"
#include "lazy_tensor_core/csrc/ts_backend/ts_lowering_context.h"
#include "lazy_tensors/computation_client/sys_util.h"
#include "torch/csrc/jit/api/function_impl.h"
#include "torch/csrc/jit/ir/ir.h"

namespace torch_lazy_tensors {
namespace ir {
using namespace torch_lazy_tensors::compiler;
using NodePtr = torch::lazy::NodePtr;
using Node = torch::lazy::Node;
using OpKind = torch::lazy::OpKind;
using OpList = torch::lazy::OpList;
using TSOpVector = std::vector<torch::jit::Value*>;

namespace ops {
  using NodePtr = torch::lazy::NodePtr;

}

// Helper that makes it easy to access the TsNode::shape() method
// from an torch::lazy::Output* that holds a Node* that points to a TsNode
// TODO(whc) remove these once migrating to codegen and cleaning up Shape use
const torch::lazy::Shape& GetShapeFromTsOutput(const torch::lazy::Output& output);
const torch::lazy::Shape& GetShapeFromTsValue(const torch::lazy::Value& value);
void TsNodeSetShapeDeferred(
    NodePtr node, const std::function<torch::lazy::Shape()>& shape_fn);

class TsNode : public torch::lazy::Node {
 public:
  TsNode(OpKind op, OpList operands, std::vector<torch::lazy::Shape>&& shapes,
         size_t num_outputs = 1, torch::lazy::hash_t hash_seed = torch::lazy::kHashSeed);

  // Same as the constructor above, but the shape is generated by a function,
  // only if needed (shape cache miss).
  TsNode(OpKind op, OpList operands,
         const std::function<torch::lazy::Shape()>& shape_fn,
         size_t num_outputs = 1, torch::lazy::hash_t hash_seed = torch::lazy::kHashSeed);

  // The shape is set later.
  TsNode(OpKind op, OpList operands, size_t num_outputs = 1,
         torch::lazy::hash_t hash_seed = torch::lazy::kHashSeed);

  void SetShapeDeferred(const std::function<torch::lazy::Shape()>& shape_fn);

  // Contructor used to create leaf nodes.
  TsNode(OpKind op, torch::lazy::Shape shape, size_t num_outputs = 1,
         torch::lazy::hash_t hash_seed = torch::lazy::kHashSeed);

  virtual ~TsNode() = default;

  torch::lazy::Shape GetOpShape(
      const std::function<torch::lazy::Shape()>& shape_fn) const;

  // Retrieves the full shape of the IR Node.
  c10::ArrayRef<torch::lazy::Shape> shapes() const { return shapes_; }

  // Retrieves the shape of the output at a given index.
  const torch::lazy::Shape& shape(size_t output_index = 0) const;

  virtual std::string ToString() const override;

  static torch::lazy::hash_t GetOpHash(OpKind op,
                                       const torch::lazy::Shape& shape,
                                       torch::lazy::hash_t hash_seed);

  virtual const std::vector<torch::lazy::Output>& operands() const override {
    return operands_as_outputs_;
  }
  virtual const torch::lazy::Output& operand(size_t i) const override {
    return operands_as_outputs_.at(i);
  }

  // Lower is a backend-specific method since it returns a backend specific
  // type. hence, it is convenient to define it differently per-backend rather
  // than at Node API
  virtual TSOpVector Lower(std::shared_ptr<torch::jit::GraphFunction> function,
                           ts_backend::TSLoweringContext* loctx) const;

 private:
  // Adds node's index output number as operand.
  void AddOperand(NodePtr node, size_t index = 0);

  std::vector<torch::lazy::Shape> shapes_;
  // A node holds a real reference to its operands.
  std::vector<NodePtr> operands_;
  // Outputs do not hold references on the nodes, and neither do the uses, since
  // otherwise we get into circular reference counting.
  std::vector<torch::lazy::Output> operands_as_outputs_;
};

}  // namespace ir
}  // namespace torch_lazy_tensors
