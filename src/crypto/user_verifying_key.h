// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRYPTO_USER_VERIFYING_KEY_H_
#define CRYPTO_USER_VERIFYING_KEY_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "crypto/crypto_export.h"
#include "crypto/signature_verifier.h"
#include "crypto/unexportable_key.h"

namespace crypto {

typedef std::string UserVerifyingKeyLabel;

// UserVerifyingSigningKey is a hardware-backed key that triggers a user
// verification by the platform before a signature will be provided.
//
// Notes:
// - This is currently only supported on Windows and Mac.
// - This does not export a wrapped key because the Windows implementation uses
//   the WinRT KeyCredentialManager which addresses stored keys by name.
// - The interface for this class will likely need to be generalized as support
//   for other platforms is added.
class CRYPTO_EXPORT UserVerifyingSigningKey {
 public:
  virtual ~UserVerifyingSigningKey();

  // Sign invokes |callback| to provide a signature of |data|, or |nullopt| if
  // an error occurs during signing.
  virtual void Sign(
      base::span<const uint8_t> data,
      base::OnceCallback<void(std::optional<std::vector<uint8_t>>)>
          callback) = 0;

  // Provides the SPKI public key.
  virtual std::vector<uint8_t> GetPublicKey() const = 0;

  // Get a reference to the label used to create or retrieve this key.
  virtual const UserVerifyingKeyLabel& GetKeyLabel() const = 0;
};

// Reference-counted wrapper for UserVeriyingSigningKey.
class CRYPTO_EXPORT RefCountedUserVerifyingSigningKey
    : public base::RefCountedThreadSafe<RefCountedUserVerifyingSigningKey> {
 public:
  explicit RefCountedUserVerifyingSigningKey(
      std::unique_ptr<crypto::UserVerifyingSigningKey> key);

  RefCountedUserVerifyingSigningKey(const RefCountedUserVerifyingSigningKey&) =
      delete;
  RefCountedUserVerifyingSigningKey& operator=(
      const RefCountedUserVerifyingSigningKey&) = delete;

  crypto::UserVerifyingSigningKey& key() const { return *key_; }

 private:
  friend class base::RefCountedThreadSafe<RefCountedUserVerifyingSigningKey>;
  ~RefCountedUserVerifyingSigningKey();

  const std::unique_ptr<crypto::UserVerifyingSigningKey> key_;
};

// UserVerifyingKeyProvider creates |UserVerifyingSigningKey|s.
// Only one call to |GenerateUserVerifyingSigningKey| or
// |GetUserVerifyingSigningKey| can be outstanding at one time for a single
// provider, but multiple providers can be used. Destroying a provider will
// cancel an outstanding key generation or retrieval and delete the callback
// without running it.
class CRYPTO_EXPORT UserVerifyingKeyProvider {
 public:
  struct Config {
#if BUILDFLAG(IS_MAC)
    // The keychain access group the key is shared with. The binary must be
    // codesigned with the corresponding entitlement.
    // https://developer.apple.com/documentation/bundleresources/entitlements/keychain-access-groups?language=objc
    // This must be set to a non empty value when using user verifying keys on
    // macOS.
    std::string keychain_access_group;
#endif  // BUILDFLAG(IS_MAC)
  };

  virtual ~UserVerifyingKeyProvider();

  // Similar to |GenerateSigningKeySlowly| but the resulting signing key can
  // only be used with a local user authentication by the platform. This can be
  // called from any thread as the work is done asynchronously on a
  // high-priority thread when the underlying platform is slow.
  // Invokes |callback| with the resulting key, or nullptr on error.
  virtual void GenerateUserVerifyingSigningKey(
      base::span<const SignatureVerifier::SignatureAlgorithm>
          acceptable_algorithms,
      base::OnceCallback<void(std::unique_ptr<UserVerifyingSigningKey>)>
          callback) = 0;

  // Similar to |FromWrappedSigningKey| but uses a wrapped key that was
  // generated from |GenerateUserVerifyingSigningKey|. This can be called from
  // any thread as the work is done asynchronously on a high-priority thread
  // when the underlying platform is slow.
  // Invokes |callback| with the resulting key, or nullptr on error.
  virtual void GetUserVerifyingSigningKey(
      UserVerifyingKeyLabel key_label,
      base::OnceCallback<void(std::unique_ptr<UserVerifyingSigningKey>)>
          callback) = 0;

  // Deletes a user verifying signing key. Work is be done asynchronously on a
  // low-priority thread when the underlying platform is slow.
  // Invokes |callback| with `true` if the key was found and deleted, `false`
  // otherwise.
  virtual void DeleteUserVerifyingKey(
      UserVerifyingKeyLabel key_label,
      base::OnceCallback<void(bool)> callback) = 0;
};

// GetUserVerifyingKeyProvider returns |UserVerifyingKeyProvider| for the
// current platform, or nullptr if this is not implemented on the current
// platform.
// Note that this will return non null if keys are supported but not available,
// i.e. if |AreUserVerifyingKeysSupported| returns false. In that case,
// operations would fail.
CRYPTO_EXPORT std::unique_ptr<UserVerifyingKeyProvider>
GetUserVerifyingKeyProvider(UserVerifyingKeyProvider::Config config);

// Invokes the callback with true if UV keys can be used on the current
// platform, and false otherwise. `callback` can be invoked synchronously or
// asynchronously.
CRYPTO_EXPORT void AreUserVerifyingKeysSupported(
    UserVerifyingKeyProvider::Config config,
    base::OnceCallback<void(bool)> callback);

namespace internal {

CRYPTO_EXPORT void SetUserVerifyingKeyProviderForTesting(
    std::unique_ptr<UserVerifyingKeyProvider> (*func)());

}  // namespace internal

}  // namespace crypto

#endif  // CRYPTO_USER_VERIFYING_KEY_H_
