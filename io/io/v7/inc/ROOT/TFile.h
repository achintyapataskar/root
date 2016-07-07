/// \file ROOT/TFile.h
/// \ingroup Base ROOT7
/// \author Axel Naumann <axel@cern.ch>
/// \date 2015-07-31
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback is welcome!

/*************************************************************************
 * Copyright (C) 1995-2016, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_TFile
#define ROOT7_TFile

#include "ROOT/TDirectory.h"

#include "TClass.h"

#include "RStringView.h"
#include <memory>
#include <typeinfo>

namespace ROOT {
namespace Experimental {

class TFilePtr;

namespace Internal {
class TFileStorageInterface;
class TFileSharedPtrCtor;
}

/** \class ROOT::Experimental::TFile
 A ROOT file.

 A ROOT file is an object store: it can serialize any
 object for which ROOT I/O is available (generally: an object which has a
 dictionary), and it stores the object's data under a key name.

 */
class TFile: public TDirectory {
private:
  std::unique_ptr<Internal::TFileStorageInterface> fStorage; ///< Storage backend.

  TFile(std::unique_ptr<Internal::TFileStorageInterface>&& storage);

  /// Serialize the object at address, using the object's TClass.
  //FIXME: what about `cl` "pointing" to a base class?
  void WriteMemoryWithType(std::string_view name, const void *address,
                           TClass *cl);

  friend Internal::TFileSharedPtrCtor;

public:

  /// Options for TFile construction.
  struct Options_t {
    /// Default constructor needed for member inits.
    Options_t() { }

    /// Whether the file should be opened asynchronously, if available.
    bool fAsynchronousOpen = false;

    /// Timeout for asynchronous opening.
    int fAsyncTimeout = 0;

    /// Whether the file should be cached before reading. Only available for
    /// "remote" file protocols. If the download fails, the file will be opened
    /// remotely.
    bool fCachedRead = false;

    /// Where to cache the file. If empty, defaults to TFilePtr::GetCacheDir().
    std::string fCacheDir;
  };

  ///\name Generator functions
  ///\{

  /// Open a file with `name` for reading.
  ///
  /// \note: Synchronizes multi-threaded accesses through locks.
  static TFilePtr Open(std::string_view name,
                       const Options_t &opts = Options_t());

  /// Open an existing file with `name` for reading and writing. If a file with
  /// that name does not exist, an invalid TFilePtr will be returned.
  ///
  /// \note: Synchronizes multi-threaded accesses through locks.
  static TFilePtr OpenForUpdate(std::string_view name,
                                const Options_t &opts = Options_t());

  /// Open a file with `name` for reading and writing. Fail (return an invalid
  /// `TFilePtr`) if a file with this name already exists.
  ///
  /// \note: Synchronizes multi-threaded accesses through locks.
  static TFilePtr Create(std::string_view name,
                         const Options_t &opts = Options_t());

  /// Open a file with `name` for reading and writing. If a file with this name
  /// already exists, delete it and create a new one. Else simply create a new file.
  ///
  /// \note: Synchronizes multi-threaded accesses through locks.
  static TFilePtr Recreate(std::string_view name,
                           const Options_t &opts = Options_t());

  ///\}

  /// Set the new directory used for cached reads, returns the old directory.
  ///
  /// \note: Synchronizes multi-threaded accesses through locks.
  static std::string SetCacheDir(std::string_view path);

  /// Get the directory used for cached reads.
  static std::string GetCacheDir();


  /// Must not call Write() of all attached objects:
  /// some might not be needed to be written or writing might be aborted due to
  /// an exception; require explicit Write().
  ~TFile();

  /// Save all objects associated with this directory (including file header) to
  /// the storage medium.
  void Flush();

  /// Flush() and make the file non-writable: close it.
  void Close();

  /// Read the object for a key. `T` must be the object's type.
  /// This will re-read the object for each call, returning a new copy; whether
  /// the `TDirectory` is managing an object attached to this key or not.
  /// \returns a `unique_ptr` to the object.
  /// \throws TDirectoryUnknownKey if no object is stored under this name.
  /// \throws TDirectoryTypeMismatch if the object stored under this name is of
  ///   a type different from `T`.
  template<class T>
  std::unique_ptr <T> Read(std::string_view name) {
    // FIXME: need separate collections for a TDirectory's key/value and registered objects. Here, we want to emit a read and must look through the key/values without attaching an object to the TDirectory.
    if (const Internal::TDirectoryEntryPtrBase *dep = Find(name.to_string())) {
      // FIXME: implement upcast!
      // FIXME: do not register read object in TDirectory
      // FIXME: implement actual read
      if (auto depT = dynamic_cast<const Internal::TDirectoryEntryPtr <T> *>(dep)) {
        //FIXME: for now, copy out of whatever the TDirectory manages.
        return std::make_unique<T>(*depT->GetPointer());
      }
      // FIXME: add expected versus actual type name as c'tor args
      throw TDirectoryTypeMismatch(name.to_string());
    }
    throw TDirectoryUnknownKey(name.to_string());
    return std::unique_ptr<T>(); // never happens
  }


  /// Write an object that is not lifetime managed by this TFileImplBase.
  template<class T>
  void Write(std::string_view name, const T &obj) {
    WriteMemoryWithType(name, &obj, TClass::GetClass(typeid(T)));
  }

  /// Write an object that is not lifetime managed by this TFileImplBase.
  template<class T>
  void Write(std::string_view name, const T *obj) {
    WriteMemoryWithType(name, obj, TClass::GetClass(typeid(T)));
  }

  /// Write an object that is already lifetime managed by this TFileImplBase.
  void Write(std::string_view name) {
    const Internal::TDirectoryEntryPtrBase *dep = Find(name.to_string());
    WriteMemoryWithType(name, dep->GetObjectAddr(), dep->GetType());
  }

  /// Hand over lifetime management of an object to this TFileImplBase, and
  /// write it.
  template<class T>
  void Write(std::string_view name, std::shared_ptr <T> &&obj) {
    Add(name, obj);
    // FIXME: use an iterator from the insertion to write instead of a second name lookup.
    Write(name);
  }
};

/**
 \class TFilePtr
 \brief Points to an object that stores or reads objects in ROOT's binary
 format.

 FIXME: implement async open; likely using std::future, possibly removing the
 Option_t element.

 */

class TFilePtr {
private:
  std::shared_ptr<TFile> fFile;

  /// Constructed by Open etc.
  TFilePtr(std::shared_ptr<TFile>&&);

  friend class TFile;

public:
  /// Dereference the file pointer, giving access to the TFileImplBase object.
  TFile* operator ->() { return fFile.get(); }

  /// Dereference the file pointer, giving access to the TFileImplBase object.
  /// const overload.
  const TFile* operator ->() const { return fFile.get(); }

  /// Check the validity of the file pointer.
  operator bool() const { return fFile.get(); }
};

} // namespace Experimental
} // namespace ROOT
#endif
