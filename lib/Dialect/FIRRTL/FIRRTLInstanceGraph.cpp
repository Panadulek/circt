//===- FIRRTLInstanceGraph.cpp - Instance Graph -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/FIRRTL/FIRRTLInstanceGraph.h"
#include "circt/Dialect/FIRRTL/FIRRTLTypes.h"
#include "mlir/IR/BuiltinOps.h"

using namespace circt;
using namespace firrtl;

/// Find all the class names referred to by a type.
static void findClassDependencies(Type type,
                                  llvm::SmallSetVector<StringAttr, 4> &deps) {
  if (auto classType = type_dyn_cast<ClassType>(type)) {
    deps.insert(classType.getNameAttr().getAttr());
  } else if (auto bundleType = type_dyn_cast<BundleType>(type)) {
    for (auto element : bundleType.getElements())
      findClassDependencies(element.type, deps);
  } else if (auto vectorType = type_dyn_cast<FVectorType>(type)) {
    findClassDependencies(vectorType.getElementType(), deps);
  }
}

static CircuitOp findCircuitOp(Operation *operation) {
  if (auto mod = dyn_cast<mlir::ModuleOp>(operation))
    for (auto &op : *mod.getBody())
      if (auto circuit = dyn_cast<CircuitOp>(&op))
        return circuit;
  return cast<CircuitOp>(operation);
}

InstanceGraph::InstanceGraph(Operation *operation)
    : igraph::InstanceGraph(findCircuitOp(operation)) {
  topLevelNode = lookup(cast<CircuitOp>(getParent()).getNameAttr());

  // Scan all modules for class dependencies hidden in types.
  for (auto *node : *this) {
    auto module = dyn_cast<FModuleLike>(*node->getModule());
    if (!module)
      continue;

    llvm::SmallSetVector<StringAttr, 4> classDeps;

    // Scan port types.
    for (auto portType : module.getPortTypes())
      findClassDependencies(cast<TypeAttr>(portType).getValue(), classDeps);

    // Scan all operations in the module.
    module->walk([&](Operation *op) {
      // InstanceOpInterface ops are already handled by the base class.
      if (isa<igraph::InstanceOpInterface>(op))
        return;

      for (auto type : op->getResultTypes())
        findClassDependencies(type, classDeps);
      for (auto type : op->getOperandTypes())
        findClassDependencies(type, classDeps);
    });

    // Add class dependencies to the graph.
    for (auto className : classDeps) {
      auto *targetNode = getOrAddNode(className);
      node->addInstance(nullptr, targetNode);
    }
  }
}
