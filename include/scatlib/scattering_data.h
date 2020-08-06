/** \file scattering_data.h
 *
 * Provides the ScatteringData class providing methods for the manipulation
 * of scattering data.
 *
 * @author Simon Pfreundschuh, 2020
 */
#ifndef __SCATLIB_SCATTERING_DATA__
#define __SCATLIB_SCATTERING_DATA__

#include <scatlib/eigen.h>
#include <scatlib/sht.h>

namespace scatlib {

template <typename Scalar, size_t rank>
using SpectralTensorType = eigen::Tensor<std::complex<Scalar>, rank>;
template <typename Scalar, size_t rank>
using GriddedTensorType = eigen::Tensor<Scalar, rank>;

namespace detail {

/** Transform fully-gridded to spectral representation.
 * Class to transform fully-gridded scattering data to spectral
 * format along the scattering angles.
 */
template <typename Scalar>
class GriddedToSpectralTransformer {
  /** Initialize transformer.
   *
   * @param t The input tensor to transform.
   */
  GriddedToSpectralTransformer(scatlib::sht::SHT &sht)
      : sht_(sht),
        n_lat_(sht.get_size_of_colatitude_grid()),
        n_lon_(sht.get_size_of_longitude_grid()),
        nlm_scat_(sht.get_number_of_spectral_coeffs()) {}

  /** Get major stride for input.
   *
   * The major stride of the input tensor is the distance between different
   * scattering-angle sequences.
   * @return the major input stride of the input data.
   */
  size_t get_major_stride_in() { return n_lat_ * n_lon_; }

  /** Get major stride for output.
   *
   * The major stride of the output tensor is the distance between different
   * sequences of spherical harmonics coefficients.
   * @return the major input stride of the input data.
   */
  size_t get_major_stride_out() { return nlm_scat_; }

  /// Map pointing to the two-dimensions coefficient field for given scattering
  /// angles.
  template <size_t rank>
  eigen::MatrixMap<Scalar> get_input_sequence(
      size_t major_index,
      GriddedTensorType<Scalar, rank> &in) {
    size_t start = major_index * get_major_stride_in();
    return eigen::MatrixMap<Scalar>(in.data, n_lon_, n_lat_);
  }

  /// Map pointing to spherical harmonics coefficient vector in input tensor.
  template <size_t rank>
  eigen::VectorMap<std::complex<Scalar>> get_output_sequence(
      size_t major_index,
      SpectralTensorType<Scalar, rank - 1> &out) {
    size_t start = major_index * get_major_stride_out();
    return eigen::VectorMap<Scalar>(out.data, nlm_scat_);
  }

  /// Dimensions of the output (transformed) tensor.
  template <size_t rank>
  std::array<size_t, rank - 1> get_output_dimensions(
      const GriddedTensorType<Scalar, rank> &in) {
    auto input_dimensions = in.dimensions();
    std::array<size_t, rank - 1> output_dimensions;
    std::copy(input_dimensions.begin(),
              input_dimensions.end() - 1,
              output_dimensions.begin());
    output_dimensions[rank - 1] = nlm_scat_;
    return output_dimensions;
  }

  template <size_t rank>
  SpectralTensorType<Scalar, rank - 1> transform(
      const SpectralTensorType<Scalar, rank> &in) {
    SpectralTensorType<Scalar, rank - 1> out(get_output_dimensions());
    for (size_t i = 0; i < out.size() / get_major_stride_out(); ++i) {
      get_output_sequence(i, out) = get_input_sequence(i, in);
    }
    return out;
  }

 protected:
  scatlib::sht::SHT &sht_;
  size_t n_lat_, n_lon_, nlm_scat_;
};

/** Transform spectral to fully-spectral.
 *
 * Class to transform spectral scattering data to fully-spectral
 * format.
 *
 */
template <typename Scalar>
class SpectralToFullySpectralTransformer {
  SpectralToFullySpectralTransformer(size_t nlm_scat, scatlib::sht::SHT &sht)
      : sht_(sht),
        n_lat_(sht.get_size_of_colatitude_grid()),
        n_lon_(sht.get_size_of_longitude_grid()),
        nlm_scat_(nlm_scat),
        nlm_inc_(sht.get_number_of_spectral_coeffs()) {}

  /**
   * The stride between sequences of SH coefficient over incoming angles
   * in the input tensor.
   */
  size_t get_major_stride_in() { return n_lat_ * n_lon_ * nlm_scat_ * 2; }

  /**
   * The stride between incoming SH coefficient-sequences in the output tensor.
   */
  size_t get_major_stride_out() { return 2 * nlm_scat_ * nlm_inc_; }

  /**
   * The stride between sequences of scattering SH-coefficients corresponding to
   * different l- and m-indices in the input tensor.
   */
  size_t get_lm_stride_in() { return nlm_scat_ * 2; }

  /**
   * The stride between sequences of incoming SH-coefficients corresponding to
   * different l- and m-indices in the input tensor.
   */
  size_t get_lm_stride_out() { return nlm_scat_; }

  /**
   * Matrix map pointing to sequence of scattering SH coefficients in the input
   * tensor.
   */
  template <size_t rank>
  eigen::MatrixMap<Scalar> get_input_sequence(
      size_t major_index,
      size_t lm_index,
      size_t complex_index,
      SpectralTensorType<Scalar, rank> &in) {
    size_t start = major_index * get_major_stride_in() +
                   lm_index * get_lm_stride_in() + complex_index;
    size_t col_stride = 2 * get_lm_stride_in();
    size_t row_stride = 2 * get_lm_stride_in() * n_lat_;
    return eigen::MatrixMap<Scalar>(in.data,
                                    n_lon_,
                                    n_lat_,
                                    {row_stride, col_stride});
  }

  /**
   * Vector map pointing to sequences of incoming SH coefficients in the input
   * tensor.
   */
  template <size_t rank>
  eigen::VectorMap<std::complex<Scalar>> get_output_sequence(
      size_t major_index,
      size_t lm_index,
      size_t complex_index,
      SpectralTensorType<Scalar, rank> &out) {
    size_t start = major_index * get_major_stride_out() +
                   complex_index * get_major_stride_out() / 2 +
                   lm_index * get_lm_stride_out();
    size_t stride = get_lm_stride_out();
    return eigen::VectorMap<Scalar>(out.data, nlm_inc_, {stride});
  }

  /// Dimensions of the output (transformed) tensor.
  template <size_t rank>
  std::array<size_t, rank> get_output_dimensions(
      SpectralTensorType<Scalar, rank> &in) {
    auto input_dimensions = in.dimensions();
    std::array<size_t, rank> output_dimensions;
    std::copy(input_dimensions.begin(),
              input_dimensions.end(),
              output_dimensions.begin());
    output_dimensions[rank - 3] = nlm_inc_;
    output_dimensions[rank - 4] = 2;
    return output_dimensions;
  }

  template <size_t rank>
  SpectralTensorType<Scalar, rank> transform() {
    SpectralTensorType<Scalar, rank> out(get_output_dimensions());
    for (size_t i = 0; i < out.size() / get_major_stride_out(); ++i) {
      for (size_t j = 0; j < nlm_scat_; ++j) {
        get_output_sequence(i, j, 0, out) = get_input_sequence(i, j, 0);
        get_output_sequence(i, j, 1, out) = get_input_sequence(i, j, 1);
      }
    }
    return out;
  }

 protected:
  scatlib::sht::SHT &sht_;
  size_t n_lat_, n_lon_, nlm_scat_, nlm_inc_;
};
}  // namespace detail

//
// Scattering data.
//

enum class DataFormat { Gridded, Spectral, FullySpectral };

/** Base class for scattering data.
 * Holds frequency and temperature grids.
 */
template <typename Scalar, DataFormat format>
class ScatteringData;

// pxx :: export
// pxx :: instance("ScatteringDataGridded", ["double"])
template <typename Scalar>
class ScatteringData<Scalar, DataFormat::Gridded> {
 public:
  using CoeffType = Scalar;
  using VectorType = eigen::Vector<Scalar>;
  template <size_t rank>
  using TensorType = eigen::Tensor<Scalar, rank>;

  ScatteringData(VectorType azimuth_grid_incoming,
                 VectorType zenith_grid_incoming,
                 VectorType azimuth_grid_scattering,
                 VectorType zenith_grid_scattering,
                 TensorType<7> phase_matrix,
                 TensorType<7> extinction_matrix,
                 TensorType<7> absorption_vector,
                 TensorType<6> backscattering_coeff,
                 TensorType<6> forwardscattering_coeff)
      : azimuth_grid_incoming_(azimuth_grid_incoming),
        zenith_grid_incoming_(zenith_grid_incoming),
        azimuth_grid_scattering_(azimuth_grid_scattering),
        zenith_grid_scattering_(zenith_grid_scattering),
        phase_matrix_(phase_matrix),
        extinction_matrix_(extinction_matrix),
        absorption_vector_(absorption_vector),
        backscattering_coeff_(backscattering_coeff),
        forwardscattering_coeff_(forwardscattering_coeff) {}

 protected:
  VectorType azimuth_grid_incoming_;
  VectorType zenith_grid_incoming_;
  VectorType azimuth_grid_scattering_;
  VectorType zenith_grid_scattering_;

  TensorType<5> phase_matrix_;       // elements x (inc. ang.) x (scat. ang.)
  TensorType<5> extinction_matrix_;  // elements x (inc. ang.) x (scat. ang.)
  TensorType<5> absorption_vector_;  // elements x (inc. ang.) x (scat. ang.)
  TensorType<4>
      backscattering_coeff_;  // elements x (inc. ang.) x (scat. ang.)
  TensorType<4> forwardscattering_coeff_;  // elements x (inc. angles) x
                                                 // (scat. ang.)
};

// pxx :: export
// pxx :: instance("ScatteringDataSpectral", ["double"])
template <typename Scalar>
class ScatteringData<Scalar, DataFormat::Spectral> {
 public:
  using CoeffType = std::complex<Scalar>;
  using VectorType = eigen::Vector<Scalar>;
  template <size_t rank>
  using TensorType = eigen::Tensor<CoeffType, rank>;

  ScatteringData(VectorType azimuth_grid_incoming,
                 VectorType zenith_grid_incoming,
                 TensorType<6> phase_matrix,
                 TensorType<6> extinction_matrix,
                 TensorType<6> absorption_vector,
                 TensorType<5> backscattering_coeff,
                 TensorType<5> forwardscattering_coeff)
      : azimuth_grid_incoming_(azimuth_grid_incoming),
        zenith_grid_incoming_(zenith_grid_incoming),
        phase_matrix_(phase_matrix_),
        extinction_matrix_(extinction_matrix_),
        absorption_vector_(absorption_vector_),
        backscattering_coeff_(backscattering_coeff),
        forwardscattering_coeff_(forwardscattering_coeff) {}

 protected:
  VectorType azimuth_grid_incoming_;
  VectorType zenith_grid_incoming_;
  TensorType<4> phase_matrix_;                   // elements x (inc. ang.) x nlm
  TensorType<4> extinction_matrix_;              // elements x (inc. ang.) x nlm
  TensorType<4> absorption_vector_;              // elements x (inc. ang.) x nlm
  TensorType<3> backscattering_coeff_;     // (inc. ang.) x nlm
  TensorType<3> forwardscattering_coeff_;  // (inc. ang.) x nlm
};

// pxx :: export
// pxx :: instance("ScatteringDataFullySpectral", ["double"])
template <typename Scalar>
class ScatteringData<Scalar, DataFormat::FullySpectral> {
 public:
  using CoeffType = std::complex<Scalar>;
  using VectorType = eigen::Vector<Scalar>;

 private:
  template <size_t rank>
  using TensorType = eigen::Tensor<CoeffType, rank>;

 public:
  ScatteringData(
                 TensorType<6> phase_matrix,
                 TensorType<6> extinction_matrix,
                 TensorType<6> absorption_vector,
                 TensorType<5> backscattering_coeff,
                 TensorType<5> forwardscattering_coeff)
      : phase_matrix_(phase_matrix),
        extinction_matrix_(extinction_matrix),
        absorption_vector_(absorption_vector),
        backscattering_coeff_(backscattering_coeff),
        forwardscattering_coeff_(forwardscattering_coeff) {}

  const TensorType<4> &get_phase_matrix() const { return phase_matrix_; }
  TensorType<4> &get_phase_matrix() { return phase_matrix_; };
  const TensorType<4> &get_extinction_matrix() const {
    return extinction_matrix_;
  };
  TensorType<4> &get_extinction_matrix() { return extinction_matrix_; };
  const TensorType<4> &get_absorption_vector() const {
    return absorption_vector_;
  };
  TensorType<4> &get_absorption_vector() { return absorption_vector_; };
  const TensorType<3> &get_backscattering_coeff() const {
    return backscattering_coeff_;
  };
  TensorType<3> &get_backscattering_coeff() {
    return backscattering_coeff_;
  }
  const TensorType<3> &get_forwardscattering_coeff() const {
    return forwardscattering_coeff_;
  }
  TensorType<3> &get_forwardscattering_coeff() {
    return forwardscattering_coeff_;
  }

 protected:
  TensorType<4> phase_matrix_;                   // elements x nlm_inc x nlm x 2
  TensorType<4> extinction_matrix_;              // elements x nlm_inc x nlm x 2
  TensorType<4> absorption_vector_;              // elements x nlm_inc x nlm x 2
  TensorType<3> backscattering_coeff_;     // nlm_inc x nlm x 2
  TensorType<3> forwardscattering_coeff_;  // nlm_inc x nlm x 2
};

}  // namespace scatlib

#endif
