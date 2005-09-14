// Copyright (C) 2003-2005 Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// First added:  2003-05-06
// Last changed: 2005-09-14
//
// These are not really macros, but we try to mimic the structure
// of the log system.

#ifndef __SETTINGS_MACROS_H
#define __SETTINGS_MACROS_H

#include <stdarg.h>

namespace dolfin
{

  /// Add new parameter
  void dolfin_parameter(Parameter::Type type, const char* key, ...);

  /// Set value of a parameter
  void dolfin_set(const char* key, ...);

  /// Set value of a parameter (aptr version)
  void dolfin_set_aptr(const char* key, va_list aptr);

  /// Get value of a parameter
  Parameter dolfin_get(const char* key);

  /// Check if parameter has been changed
  bool dolfin_parameter_changed(const char* key);
  
  /// Load parameters from given file
  void dolfin_load(const char* filename);

  /// Save parameters to given file
  void dolfin_save(const char* filename);

}

#endif
