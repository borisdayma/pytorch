#pragma once

#include <ATen/Tensor.h>

namespace torch {
namespace autograd {
namespace utils {

// Helper functions to enforce the "Gradient Layout Contract" described in
// torch/csrc/autograd/AccumulateGrad.h.

// Checks if grad obeys the contract with variable.
inline bool obeys_layout_contract(const at::Tensor& grad, const at::Tensor& variable) {
  TORCH_INTERNAL_ASSERT(!grad.is_sparse());
  TORCH_INTERNAL_ASSERT(!variable.is_sparse());
  if (variable.is_non_overlapping_and_dense()) {
    // Only look at stride for dimensions that are not of size 1
    const auto& grad_sizes = grad.sizes();
    const auto& grad_strides = grad.strides();
    const auto& variable_strides = variable.strides();
    for (const auto idx : c10::irange(grad_sizes.size())) {
      if (grad_sizes[idx] != 1) {
        if (grad_strides[idx] != variable_strides[idx]) {
          return false;
        }
      }
    }
    return true;
  } else {
    return grad.is_contiguous(at::MemoryFormat::Contiguous);
  }
}

// Creates a clone of new_grad that obeys the contract with variable.
// The clone should attach to new_grad's history if GradMode::is_enabled().
inline at::Tensor clone_obey_contract(const at::Tensor& new_grad, const at::Tensor& variable) {
  if (variable.is_non_overlapping_and_dense()) {
    // (1)
    // Does this dicey-looking sequence attach the result to new_grad's
    // history if GradMode::is_enabled()?  Yes, and @alband says it should.
    return std::move(new_grad.new_empty_strided(variable.sizes(), variable.strides(),
                                       variable.options().memory_format(c10::nullopt))
                     .copy_(new_grad));
  } else {
    // (2)
    return new_grad.clone(at::MemoryFormat::Contiguous);
  }
}

} // namespace utils
} // namespace autograd
} // namespace torch
