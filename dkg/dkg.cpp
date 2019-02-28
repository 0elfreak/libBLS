#include <dkg/dkg.h>

#include <boost/multiprecision/cpp_int.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/exponentiation/exponentiation.hpp>

namespace signatures {

  typedef std::vector<libff::alt_bn128_Fr> Polynomial;

  dkg::dkg(const size_t t, const size_t n) : t(t), n(n) {
    libff::init_alt_bn128_params();  // init all libff::alt_bn128 parameters
  }

  Polynomial dkg::GeneratePolynomial() {
    // generate polynomial of degree t for each node that takes part in DKG
    Polynomial pol(this->t);

    for (size_t i = 0; i < this->t; ++i) {
      pol[i] = libff::alt_bn128_Fr::random_element();

      while (i == this->t - 1 && pol[i] == libff::alt_bn128_Fr::zero()) {
        pol[i] = libff::alt_bn128_Fr::random_element();
      }
    }

    return pol;
  }

  std::vector<libff::alt_bn128_G2> dkg::VerificationVector(const std::vector<libff::alt_bn128_Fr>& polynomial) {
    // vector of public values that each node will broadcast
    std::vector<libff::alt_bn128_G2> verification_vector(this->t);
    for (size_t i = 0; i < this->t; ++i) {
      verification_vector[i] = polynomial[i] * libff::alt_bn128_G2::one();
    }

    return verification_vector;
  }

  libff::alt_bn128_Fr dkg::PolynomialValue(const Polynomial& pol, libff::alt_bn128_Fr point) {
    // calculate value of polynomial in a random integer point
    libff::alt_bn128_Fr value = libff::alt_bn128_Fr::zero();

    libff::alt_bn128_Fr pow = libff::alt_bn128_Fr::one();
    for (size_t i = 0; i < this->t; ++i) {
      if (i == this->t - 1 && pol[i] == libff::alt_bn128_Fr::zero()) {
        throw std::runtime_error("Error, incorrect degree of a polynomial");
      }
      value += pol[i] * pow;
      pow *= point;
    }

    return value;
  }

  std::vector<libff::alt_bn128_Fr> dkg::SecretKeyContribution(const std::vector<libff::alt_bn128_Fr>& polynomial) {
    // calculate for each node a list of secret values that will be used for verification
    std::vector<libff::alt_bn128_Fr> secret_key_contribution(this->n);
    for (size_t i = 0; i < this->n; ++i) {
      secret_key_contribution[i] = PolynomialValue(polynomial, libff::alt_bn128_Fr(i + 1));
    }

    return secret_key_contribution;
  }

  libff::alt_bn128_Fr dkg::SecretKeyShareCreate(const std::vector<libff::alt_bn128_Fr>& secret_key_contribution) {
    // create secret key share from secret key contribution
    libff::alt_bn128_Fr secret_key_share = libff::alt_bn128_Fr::zero();

    for (size_t i = 0; i < this->n; ++i) {
      secret_key_share = secret_key_share + secret_key_contribution[i];
    }

    if (secret_key_share == libff::alt_bn128_Fr::zero()) {
      throw std::runtime_error("Error, at least one secret key share is equal to zero");
    }

    return secret_key_share;
  }

  bool dkg::Verification(size_t idx, libff::alt_bn128_Fr share,
                        const std::vector<libff::alt_bn128_G2>& verification_vector) {
    // verifies that idx-th node is not broken
    libff::alt_bn128_G2 value = libff::alt_bn128_G2::zero();
    for (size_t i = 0; i < this->t; ++i) {
      value = value + power(libff::alt_bn128_Fr(idx + 1), i) * verification_vector[i];
    }

    return (value == share * libff::alt_bn128_G2::one());
  }

}  // namespace signatures
