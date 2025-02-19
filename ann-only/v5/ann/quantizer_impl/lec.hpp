/*
 * Copyright 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance
 * with the License. A copy of the License is located at
 *
 * http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */

#include <algorithm>
#include <limits>
#include <vector>
#include "utils/clustering.hpp"

namespace pecos {

namespace ann {

    struct ProductQuantizer4Bits {
        const size_t num_of_local_centroids = 16;
        index_type num_local_codebooks;
        int local_dimension;
        std::vector<float> global_centroid;
        std::vector<float> local_codebooks;
        std::vector<float> original_local_codebooks;

        void save(FILE* fp) const {
            pecos::file_util::fput_multiple<index_type>(&num_local_codebooks, 1, fp);
            pecos::file_util::fput_multiple<int>(&local_dimension, 1, fp);
            size_t sz = global_centroid.size();
            pecos::file_util::fput_multiple<size_t>(&sz, 1, fp);
            if (sz) {
                pecos::file_util::fput_multiple<float>(&global_centroid[0], sz, fp);
            }
            sz = original_local_codebooks.size();
            pecos::file_util::fput_multiple<size_t>(&sz, 1, fp);
            if (sz) {
                pecos::file_util::fput_multiple<float>(&original_local_codebooks[0], sz, fp);
            }
            sz = local_codebooks.size();
            pecos::file_util::fput_multiple<size_t>(&sz, 1, fp);
            if (sz) {
                pecos::file_util::fput_multiple<float>(&local_codebooks[0], sz, fp);
            }
        }

        void load(FILE* fp) {
            pecos::file_util::fget_multiple<index_type>(&num_local_codebooks, 1, fp);
            pecos::file_util::fget_multiple<int>(&local_dimension, 1, fp);
            size_t sz = 0;
            pecos::file_util::fget_multiple<size_t>(&sz, 1, fp);
            global_centroid.resize(sz);
            if (sz) {
                pecos::file_util::fget_multiple<float>(&global_centroid[0], sz, fp);
            }
            pecos::file_util::fget_multiple<size_t>(&sz, 1, fp);
            original_local_codebooks.resize(sz);
            if (sz) {
                pecos::file_util::fget_multiple<float>(&original_local_codebooks[0], sz, fp);
            }
            pecos::file_util::fget_multiple<size_t>(&sz, 1, fp);
            local_codebooks.resize(sz);
            if (sz) {
                pecos::file_util::fget_multiple<float>(&local_codebooks[0], sz, fp);
            }
        }

        void pack_codebook_for_inference() {
            local_codebooks = original_local_codebooks;
        }

        void pad_parameters(index_type& max_degree, index_type& code_dimension) {}

        void encode(float* query, uint8_t* codes) {
            for (index_type d = 0; d < num_local_codebooks; d++) {
                std::vector<float > results;
                for (size_t k = 0; k < num_of_local_centroids; k++) {
                    float v = 0;
                    for (int j = 0; j < local_dimension; j++) {
                        float tmp_v = original_local_codebooks[d * num_of_local_centroids * local_dimension + k * local_dimension + j]
                            - (query[d * local_dimension + j] - global_centroid[d * local_dimension + j]);
                        v += (tmp_v * tmp_v);
                    }
                    results.push_back(v);
                }
                std::vector<float>::iterator argmin_result = std::min_element(results.begin(), results.end());
                codes[d] = std::distance(results.begin(), argmin_result);
            }
        }

        void compute_centroids(pecos::drm_t& X, int dsub, int ksub, index_type *assign, float *centroids, int threads=1) {
            // zero initialization for later do_axpy
            memset(centroids, 0, ksub * dsub * sizeof(*centroids));
            std::vector<float> centroids_size(ksub);
            #pragma omp parallel num_threads(threads)
            {
                // each thread takes care of [c_l, c_r)
                int rank = omp_get_thread_num();
                size_t c_l = (ksub * rank) / threads;
                size_t c_r = (ksub * (rank + 1)) / threads;
                for (size_t i = 0; i < X.rows; i++) {
                    auto ci = assign[i];
                    if (ci >= c_l && ci < c_r) {
                        float* y = centroids + ci * dsub;
                        const auto& xi = X.get_row(i);
                        pecos::do_axpy(1.0, xi.val, y, dsub);
                        centroids_size[ci] += 1;
                    }
                }
                // normalize center vector
                for (size_t ci = c_l; ci < c_r; ci++) {
                    float* y = centroids + ci * dsub;
                    pecos::do_scale(1.0 / centroids_size[ci], y, dsub);
                }
            }
        }

        void train(const pecos::drm_t& X_trn, int M, size_t sub_sample_points=0, int seed=0, size_t max_iter=10, int threads=32) {
            size_t dimension = X_trn.cols;
            std::cout<< "step 3" <<std::endl;
            if (dimension % M != 0) {
                throw std::runtime_error("Original dimension must be divided by subspace dimension");
            }
            num_local_codebooks = M;
            local_dimension = dimension / num_local_codebooks;
            index_type n_data = X_trn.rows;
            if (sub_sample_points == 0) {
                sub_sample_points = n_data;
            }

            std::vector<float> centroids;
            original_local_codebooks.resize(num_local_codebooks * num_of_local_centroids * dimension);
            global_centroid.resize(dimension, 0);

            std::vector<float> xslice(sub_sample_points * local_dimension);
            for (index_type m = 0; m < num_local_codebooks; m++) {
                std::vector<size_t> indices(n_data, 0);
                std::iota(indices.data(), indices.data() + n_data, 0);
                std::random_shuffle(indices.data(), indices.data() + n_data);
                for (size_t i = 0; i < sub_sample_points; i++) {
                    size_t index = indices[i];
                    std::memcpy(xslice.data() + i * local_dimension, X_trn.val + index * dimension + m * local_dimension, local_dimension * sizeof(float));
                }
                pecos::drm_t Xsub;
                Xsub.rows = sub_sample_points;
                Xsub.cols = local_dimension;
                Xsub.val = xslice.data();

                // fit HLT or flat-Kmeans for each sub-space
                std::vector<index_type> assignments(sub_sample_points);
                pecos::clustering::Tree hlt(4);
                hlt.run_clustering<pecos::drm_t, index_type>(
                    Xsub,
                    0,
                    seed,
                    assignments.data(),
                    max_iter,
                    threads);

                compute_centroids(Xsub, local_dimension, num_of_local_centroids, assignments.data(),
                    &original_local_codebooks[m * num_of_local_centroids * local_dimension], threads);
            }
            pack_codebook_for_inference();
        }

        inline void approximate_neighbor_group_distance(size_t neighbor_size, float* ds, const char* neighbor_codes, uint8_t* lut_ptr, float scale, float bias) const {
            index_type num_groups = neighbor_size % 16 == 0 ? neighbor_size / 16 : neighbor_size / 16 + 1;

            std::vector<uint32_t> d(num_of_local_centroids);
            int ptr = 0;

            const uint8_t *localID = reinterpret_cast<const uint8_t*>(neighbor_codes);
            for (index_type iters = 0; iters < num_groups; iters++) {
                memset(d.data(), 0, sizeof(uint32_t) * num_of_local_centroids);
                uint8_t* local_lut_ptr = lut_ptr;
                for (index_type i = 0; i < num_local_codebooks; i++) {
                    for (size_t k = 0; k < num_of_local_centroids; k++) {
                        uint8_t obj = *localID;
                        if (k % 2 == 0) {
                            obj &= 0x0f;
                        } else {
                            obj >>= 4;
                            localID++;
                        }
                        d[k] += *(local_lut_ptr + obj);
                    }

                    local_lut_ptr += num_of_local_centroids;
                }
                for (size_t k = 0; k < num_of_local_centroids; k++) {
                    ds[k + ptr] =  d[k] * scale + bias;
                }
                ptr += num_of_local_centroids;
            }
        }

        inline void setup_lut(float* query, uint8_t* lut, float& scale, float& bias) const {
            float min = std::numeric_limits<float>::max();
            float max = std::numeric_limits<float>::min();
            // first iteration to calculate raw distance and max,min values for quantized lut
            std::vector<float> raw_dist(num_local_codebooks * num_of_local_centroids, 0);
            std::vector<float> qs(local_dimension);
            for (index_type d = 0; d < num_local_codebooks; d++) {
                for (int j = 0; j < local_dimension; j++) {
                    qs[j] = query[d * local_dimension + j] - global_centroid[d * local_dimension + j];
                }
                for (size_t k = 0; k < num_of_local_centroids; k++) {
                    float tmp_v = 0;
                    for (int j = 0; j < local_dimension; j++) {
                        float v = (qs[j] - local_codebooks[d * num_of_local_centroids * local_dimension + k * local_dimension + j]);
                        tmp_v += (v * v);
                    }
                    raw_dist[d * num_of_local_centroids + k] = tmp_v;
                    max = std::max(max, tmp_v);
                    min = std::min(min, tmp_v);
                }
            }

            bias = min;
            scale = (max - min) / 255.0;
            // second iteration to calculate quantized distnace and put it into lut
            for (index_type d = 0; d < num_local_codebooks; d++) {
                for (size_t k = 0; k < num_of_local_centroids; k++) {
                    lut[d * num_of_local_centroids + k] = std::round((raw_dist[d * num_of_local_centroids + k] - bias) / scale);
                }
            }
        }


    };

}  // end of namespace ann
}  // end of namespace pecos

