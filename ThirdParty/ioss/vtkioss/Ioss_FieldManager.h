// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Field.h> // for Field, Field::RoleType
#include <Ioss_Utils.h> // for Utils
#include <cstddef>      // for size_t
#include <functional>   // for binary_function
#include <string>       // for string
#include <unordered_map>
#include <vector> // for vector

namespace Ioss {
  using FieldMapType   = std::unordered_map<std::string, Field>;
  using FieldValuePair = FieldMapType::value_type;

  /** \brief A collection of Ioss::Field objects.
   */
  class FieldManager
  {
  public:
    FieldManager() = default;
    FieldManager(const FieldManager &other) : fields(other.fields) {}
    FieldManager &operator=(const FieldManager &) = delete;
    ~FieldManager()                               = default;

    // If a field with the same 'name' exists, an exception will be thrown.
    // Add the specified field to the list.
    void add(const Field &new_field);

    // Remove all fields of type `role`
    void erase(Field::RoleType role);

    // Assumes: Field 'name' must exist.
    void erase(const std::string &field_name);

    // Checks if a field with 'field_name' exists in the database.
    bool exists(const std::string &field_name) const;

    Field        get(const std::string &field_name) const;
    const Field &getref(const std::string &field_name) const;

    // Returns the names of all fields
    int      describe(NameList *names) const;
    NameList describe() const;

    // Returns the names of all fields with the specified 'RoleType'
    int      describe(Field::RoleType role, NameList *names) const;
    NameList describe(Field::RoleType role) const;

    size_t count() const;

  private:
    FieldMapType fields;
#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif
  };
} // namespace Ioss
