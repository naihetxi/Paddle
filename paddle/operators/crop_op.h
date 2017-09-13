/* Copyright (c) 2016 CropdleCropdle Authors. All Rights Reserve.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#pragma once

#include "paddle/framework/eigen.h"
#include "paddle/framework/op_registry.h"

namespace paddle {
namespace operators {  // Internal

template <typename T, size_t D, int MajorType = Eigen::RowMajor,
          typename IndexType = Eigen::DenseIndex>
using EigenTensor = framework::EigenTensor<T, D, MajorType, IndexType>;

using Tensor = framework::Tensor;

int64_t transIndex(std::vector<int64_t> out_shape, std::vector<int64_t> x_shape,
                   std::vector<std::pair<int, int>> crop_rules, size_t index) {
  int64_t dim_size = out_shape.size();
  int64_t pos[dim_size];

  for (int64_t i = out_shape.size() - 1; i >= 0; --i) {
    pos[i] = (index % out_shape[i]) + crop_rules[i].first;
    index = index / out_shape[i];
  }

  size_t result = pos[0];
  for (size_t i = 1; i < x_shape.size(); ++i) {
    result = result * x_shape[i] + pos[i];
  }
  return result;
}

template <typename Place, typename T, size_t D>
void CropGradFunction(const framework::ExecutionContext& context) {
  auto* d_out = context.Input<Tensor>(framework::GradVarName("Out"));
  auto* d_x = context.Output<Tensor>(framework::GradVarName("X"));
  d_x->mutable_data<T>(context.GetPlace());
  auto d_x_dims = d_x->dims();
  auto d_out_dims = d_out->dims();

  auto offsets = context.op().Attr<std::vector<int>>("offsets");

  Eigen::array<std::pair<int, int>, D> paddings;
  for (int i = 0; i < d_out_dims.size(); ++i) {
    paddings[i].first = offsets[i];
    paddings[i].second = d_x_dims[i] - d_out_dims[i] - offsets[i];
  }

  auto d_x_tensor = EigenTensor<T, D>::From(*d_x);
  auto d_out_tensor = EigenTensor<T, D>::From(*d_out);
  auto place = context.GetEigenDevice<Place>();
  d_x_tensor.device(place) = d_out_tensor.pad(paddings, 0);
}

template <typename Place, typename T>
class CropGradKernel : public framework::OpKernel {
 public:
  void Compute(const framework::ExecutionContext& context) const override {
    size_t rank =
        context.Input<Tensor>(framework::GradVarName("Out"))->dims().size();
    switch (rank) {
      case 1:
        CropGradFunction<Place, T, 1>(context);
        break;
      case 2:
        CropGradFunction<Place, T, 2>(context);
        break;
      case 3:
        CropGradFunction<Place, T, 3>(context);
        break;
      case 4:
        CropGradFunction<Place, T, 4>(context);
        break;
      case 5:
        CropGradFunction<Place, T, 5>(context);
        break;
      case 6:
        CropGradFunction<Place, T, 6>(context);
        break;
      default:
        PADDLE_THROW(
            "CropOp only support tensors with no more than 6 dimensions.");
    }
  }
};

}  // namespace operators
}  // namespace paddle
