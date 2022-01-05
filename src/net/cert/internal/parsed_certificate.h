
// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_INTERNAL_PARSED_CERTIFICATE_H_
#define NET_CERT_INTERNAL_PARSED_CERTIFICATE_H_

#include <map>
#include <memory>
#include <vector>

#include "base/check.h"
#include "base/memory/ref_counted.h"
#include "net/base/net_export.h"
#include "net/cert/internal/certificate_policies.h"
#include "net/cert/internal/parse_certificate.h"
#include "net/der/input.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/boringssl/src/include/openssl/base.h"

namespace net {

struct GeneralNames;
class NameConstraints;
class ParsedCertificate;
class SignatureAlgorithm;
class CertErrors;

using ParsedCertificateList = std::vector<scoped_refptr<ParsedCertificate>>;

// Represents an X.509 certificate, including Certificate, TBSCertificate, and
// standard extensions.
// Creating a ParsedCertificate does not completely parse and validate the
// certificate data. Presence of a member in this class implies the DER was
// parsed successfully to that level, but does not imply the contents of that
// member are valid, unless otherwise specified. See the documentation for each
// member or the documentation of the type it returns.
class NET_EXPORT ParsedCertificate
    : public base::RefCountedThreadSafe<ParsedCertificate> {
 public:
  // Map from OID to ParsedExtension.
  using ExtensionsMap = std::map<der::Input, ParsedExtension>;

  // Creates a ParsedCertificate given a DER-encoded Certificate. Returns
  // nullptr on failure. Failure will occur if the standard certificate fields
  // and supported extensions cannot be parsed.
  // On either success or failure, if |errors| is non-null it may have error
  // information added to it.
  static scoped_refptr<ParsedCertificate> Create(
      bssl::UniquePtr<CRYPTO_BUFFER> cert_data,
      const ParseCertificateOptions& options,
      CertErrors* errors);

  // Creates a ParsedCertificate by copying the provided |data|, and appends it
  // to |chain|. Returns true if the certificate was successfully parsed and
  // added. If false is return, |chain| is unmodified.
  //
  // On either success or failure, if |errors| is non-null it may have error
  // information added to it.
  static bool CreateAndAddToVector(
      bssl::UniquePtr<CRYPTO_BUFFER> cert_data,
      const ParseCertificateOptions& options,
      std::vector<scoped_refptr<net::ParsedCertificate>>* chain,
      CertErrors* errors);

  ParsedCertificate(const ParsedCertificate&) = delete;
  ParsedCertificate& operator=(const ParsedCertificate&) = delete;

  // Returns the DER-encoded certificate data for this cert.
  const der::Input& der_cert() const { return cert_; }

  // Accessors for raw fields of the Certificate.
  const der::Input& tbs_certificate_tlv() const { return tbs_certificate_tlv_; }

  const der::Input& signature_algorithm_tlv() const {
    return signature_algorithm_tlv_;
  }

  const der::BitString& signature_value() const { return signature_value_; }

  // Accessor for struct containing raw fields of the TbsCertificate.
  const ParsedTbsCertificate& tbs() const { return tbs_; }

  // Returns the signatureAlgorithm of the Certificate (not the tbsCertificate).
  const SignatureAlgorithm& signature_algorithm() const {
    DCHECK(signature_algorithm_);
    return *signature_algorithm_;
  }

  // Returns the DER-encoded raw subject value (including the outer sequence
  // tag). This is guaranteed to be valid DER, though the contents of unhandled
  // string types are treated as raw bytes.
  der::Input subject_tlv() const { return tbs_.subject_tlv; }
  // Returns the DER-encoded normalized subject value (not including outer
  // Sequence tag). This is guaranteed to be valid DER, though the contents of
  // unhandled string types are treated as raw bytes.
  der::Input normalized_subject() const {
    return der::Input(&normalized_subject_);
  }
  // Returns the DER-encoded raw issuer value (including the outer sequence
  // tag). This is guaranteed to be valid DER, though the contents of unhandled
  // string types are treated as raw bytes.
  der::Input issuer_tlv() const { return tbs_.issuer_tlv; }
  // Returns the DER-encoded normalized issuer value (not including outer
  // Sequence tag). This is guaranteed to be valid DER, though the contents of
  // unhandled string types are treated as raw bytes.
  der::Input normalized_issuer() const {
    return der::Input(&normalized_issuer_);
  }

  // Returns true if the certificate has a BasicConstraints extension.
  bool has_basic_constraints() const { return has_basic_constraints_; }

  // Returns the ParsedBasicConstraints struct. Caller must check
  // has_basic_constraints() before accessing this.
  const ParsedBasicConstraints& basic_constraints() const {
    DCHECK(has_basic_constraints_);
    return basic_constraints_;
  }

  // Returns true if the certificate has a KeyUsage extension.
  bool has_key_usage() const { return has_key_usage_; }

  // Returns the KeyUsage BitString. Caller must check
  // has_key_usage() before accessing this.
  const der::BitString& key_usage() const {
    DCHECK(has_key_usage_);
    return key_usage_;
  }

  // Returns true if the certificate has a ExtendedKeyUsage extension.
  bool has_extended_key_usage() const { return has_extended_key_usage_; }

  // Returns the ExtendedKeyUsage key purpose OIDs. Caller must check
  // has_extended_key_usage() before accessing this.
  const std::vector<der::Input>& extended_key_usage() const {
    DCHECK(has_extended_key_usage_);
    return extended_key_usage_;
  }

  // Returns true if the certificate has a SubjectAltName extension.
  bool has_subject_alt_names() const { return subject_alt_names_ != nullptr; }

  // Returns the ParsedExtension struct for the SubjectAltName extension.
  // If the cert did not have a SubjectAltName extension, this will be a
  // default-initialized ParsedExtension struct.
  const ParsedExtension& subject_alt_names_extension() const {
    return subject_alt_names_extension_;
  }

  // Returns the GeneralNames class parsed from SubjectAltName extension, or
  // nullptr if no SubjectAltName extension was present.
  const GeneralNames* subject_alt_names() const {
    return subject_alt_names_.get();
  }

  // Returns true if the certificate has a NameConstraints extension.
  bool has_name_constraints() const { return name_constraints_ != nullptr; }

  // Returns the parsed NameConstraints extension. Must not be called if
  // has_name_constraints() is false.
  const NameConstraints& name_constraints() const {
    DCHECK(name_constraints_);
    return *name_constraints_;
  }

  // Returns true if the certificate has an AuthorityInfoAccess extension.
  bool has_authority_info_access() const { return has_authority_info_access_; }

  // Returns the ParsedExtension struct for the AuthorityInfoAccess extension.
  const ParsedExtension& authority_info_access_extension() const {
    return authority_info_access_extension_;
  }

  // Returns any caIssuers URIs from the AuthorityInfoAccess extension.
  const std::vector<base::StringPiece>& ca_issuers_uris() const {
    return ca_issuers_uris_;
  }

  // Returns any OCSP URIs from the AuthorityInfoAccess extension.
  const std::vector<base::StringPiece>& ocsp_uris() const { return ocsp_uris_; }

  // Returns true if the certificate has a Policies extension.
  bool has_policy_oids() const { return has_policy_oids_; }

  // Returns the policy OIDs. Caller must check has_policy_oids() before
  // accessing this.
  const std::vector<der::Input>& policy_oids() const {
    DCHECK(has_policy_oids());
    return policy_oids_;
  }

  // Returns true if the certificate has a PolicyConstraints extension.
  bool has_policy_constraints() const { return has_policy_constraints_; }

  // Returns the ParsedPolicyConstraints struct. Caller must check
  // has_policy_constraints() before accessing this.
  const ParsedPolicyConstraints& policy_constraints() const {
    DCHECK(has_policy_constraints_);
    return policy_constraints_;
  }

  // Returns true if the certificate has a PolicyMappings extension.
  bool has_policy_mappings() const { return has_policy_mappings_; }

  // Returns the PolicyMappings extension. Caller must check
  // has_policy_mappings() before accessing this.
  const std::vector<ParsedPolicyMapping>& policy_mappings() const {
    DCHECK(has_policy_mappings_);
    return policy_mappings_;
  }

  // Returns true if the certificate has a InhibitAnyPolicy extension.
  bool has_inhibit_any_policy() const { return has_inhibit_any_policy_; }

  // Returns the Inhibit Any Policy extension. Caller must check
  // has_inhibit_any_policy() before accessing this.
  uint8_t inhibit_any_policy() const {
    DCHECK(has_inhibit_any_policy_);
    return inhibit_any_policy_;
  }

  // Returns the AuthorityKeyIdentifier extension, or nullopt if there wasn't
  // one.
  const absl::optional<ParsedAuthorityKeyIdentifier>& authority_key_identifier()
      const {
    return authority_key_identifier_;
  }

  // Returns the SubjectKeyIdentifier extension, or nullopt if there wasn't
  // one.
  const absl::optional<der::Input>& subject_key_identifier() const {
    return subject_key_identifier_;
  }

  // Returns a map of all the extensions in the certificate.
  const ExtensionsMap& extensions() const { return extensions_; }

  // Gets the value for extension matching |extension_oid|. Returns false if the
  // extension is not present.
  bool GetExtension(const der::Input& extension_oid,
                    ParsedExtension* parsed_extension) const;

 private:
  friend class base::RefCountedThreadSafe<ParsedCertificate>;
  ParsedCertificate();
  ~ParsedCertificate();

  // The backing store for the certificate data.
  bssl::UniquePtr<CRYPTO_BUFFER> cert_data_;

  // Points to the raw certificate DER.
  der::Input cert_;

  der::Input tbs_certificate_tlv_;
  der::Input signature_algorithm_tlv_;
  der::BitString signature_value_;
  ParsedTbsCertificate tbs_;

  // The signatureAlgorithm from the Certificate.
  std::unique_ptr<SignatureAlgorithm> signature_algorithm_;

  // Normalized DER-encoded Subject (not including outer Sequence tag).
  std::string normalized_subject_;
  // Normalized DER-encoded Issuer (not including outer Sequence tag).
  std::string normalized_issuer_;

  // BasicConstraints extension.
  bool has_basic_constraints_ = false;
  ParsedBasicConstraints basic_constraints_;

  // KeyUsage extension.
  bool has_key_usage_ = false;
  der::BitString key_usage_;

  // ExtendedKeyUsage extension.
  bool has_extended_key_usage_ = false;
  std::vector<der::Input> extended_key_usage_;

  // Raw SubjectAltName extension.
  ParsedExtension subject_alt_names_extension_;
  // Parsed SubjectAltName extension.
  std::unique_ptr<GeneralNames> subject_alt_names_;

  // NameConstraints extension.
  std::unique_ptr<NameConstraints> name_constraints_;

  // AuthorityInfoAccess extension.
  bool has_authority_info_access_ = false;
  ParsedExtension authority_info_access_extension_;
  // CaIssuers and Ocsp URIs parsed from the AuthorityInfoAccess extension. Note
  // that the AuthorityInfoAccess may have contained other AccessDescriptions
  // which are not represented here.
  std::vector<base::StringPiece> ca_issuers_uris_;
  std::vector<base::StringPiece> ocsp_uris_;

  // Policies extension.
  bool has_policy_oids_ = false;
  std::vector<der::Input> policy_oids_;

  // Policy constraints extension.
  bool has_policy_constraints_ = false;
  ParsedPolicyConstraints policy_constraints_;

  // Policy mappings extension.
  bool has_policy_mappings_ = false;
  std::vector<ParsedPolicyMapping> policy_mappings_;

  // Inhibit Any Policy extension.
  bool has_inhibit_any_policy_ = false;
  uint8_t inhibit_any_policy_;

  // AuthorityKeyIdentifier extension.
  absl::optional<ParsedAuthorityKeyIdentifier> authority_key_identifier_;

  // SubjectKeyIdentifier extension.
  absl::optional<der::Input> subject_key_identifier_;

  // All of the extensions.
  ExtensionsMap extensions_;
};

}  // namespace net

#endif  // NET_CERT_INTERNAL_PARSED_CERTIFICATE_H_
