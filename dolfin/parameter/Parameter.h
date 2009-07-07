// Copyright (C) 2009 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2009-05-08
// Last changed: 2009-06-30

#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <set>
#include <string>
#include <dolfin/common/types.h>

namespace dolfin
{

  /// Base class for parameters.

  class Parameter
  {
  public:

    /// Create parameter for given key
    Parameter(std::string key);

    /// Destructor
    virtual ~Parameter();

    /// Return parameter key
    std::string key() const;

    /// Return parameter description
    std::string description() const;

    /// Return access count (number of times parameter has been accessed)
    uint access_count() const;

    /// Return change count (number of times parameter has been changed)
    uint change_count() const;

    /// Set range for int-valued parameter
    virtual void set_range(int min_value, int max_value);

    /// Set range for double-valued parameter
    virtual void set_range(double min_value, double max_value);

    /// Set range for string-valued parameter
    virtual void set_range(const std::set<std::string>& range);

    /// Assignment from int
    virtual const Parameter& operator= (int value);

    /// Assignment from double
    virtual const Parameter& operator= (double value);

    /// Assignment from string
    virtual const Parameter& operator= (std::string value);

    /// Assignment from string
    virtual const Parameter& operator= (const char* value);

    /// Assignment from bool
    virtual const Parameter& operator= (bool value);

    /// Cast parameter to int
    virtual operator int() const;

    /// Cast parameter to uint
    virtual operator dolfin::uint() const;

    /// Cast parameter to double
    virtual operator double() const;

    /// Cast parameter to string
    virtual operator std::string() const;

    /// Cast parameter to bool
    virtual operator bool() const;

    /// Return value type string
    virtual std::string type_str() const = 0;

    /// Return value string
    virtual std::string value_str() const = 0;

    /// Return range string
    virtual std::string range_str() const = 0;

    /// Return short string description
    virtual std::string str() const = 0;
    
    // Check that key name is allowed
    static void check_key(std::string key);

  protected:

    // Access count
    mutable uint _access_count;

    // Change count
    uint _change_count;

  private:

    // Parameter key
    std::string _key;

    // Parameter description
    std::string _description;

  };

  /// Parameter with value type int
  class IntParameter : public Parameter
  {
  public:

    /// Create int-valued parameter
    IntParameter(std::string key, int value);

    /// Destructor
    ~IntParameter();

    /// Set range
    void set_range(int min_value, int max_value);

    /// Assignment
    const IntParameter& operator= (int value);

    /// Cast parameter to int
    operator int() const;

    /// Cast parameter to uint
    operator dolfin::uint() const;

    /// Return value type string
    std::string type_str() const;

    /// Return value string
    std::string value_str() const;

    /// Return range string
    std::string range_str() const;

    /// Return short string description
    std::string str() const;

  private:

    /// Parameter value
    int _value;

    /// Parameter range
    int _min, _max;

  };

  /// Parameter with value type double
  class DoubleParameter : public Parameter
  {
  public:

    /// Create double-valued parameter
    DoubleParameter(std::string key, double value);

    /// Destructor
    ~DoubleParameter();

    /// Set range
    void set_range(double min_value, double max_value);

    /// Assignment
    const DoubleParameter& operator= (double value);

    /// Cast parameter to double
    operator double() const;

    /// Return value type string
    std::string type_str() const;

    /// Return value string
    std::string value_str() const;

    /// Return range string
    std::string range_str() const;

    /// Return short string description
    std::string str() const;

  private:

    /// Parameter value
    double _value;

    /// Parameter range
    double _min, _max;

  };

  /// Parameter with value type string
  class StringParameter : public Parameter
  {
  public:

    /// Create string-valued parameter
    StringParameter(std::string key, std::string value);

    /// Destructor
    ~StringParameter();

    /// Set range
    void set_range(const std::set<std::string>& range);

    /// Assignment
    const StringParameter& operator= (std::string value);

    /// Assignment
    const StringParameter& operator= (const char* value);

    /// Cast parameter to string
    operator std::string() const;

    /// Return value type string
    std::string type_str() const;

    /// Return value string
    std::string value_str() const;

    /// Return range string
    std::string range_str() const;

    /// Return short string description
    std::string str() const;

  private:

    /// Parameter value
    std::string _value;

    /// Parameter range
    std::set<std::string> _range;

  };

  /// Parameter with value type bool
  class BoolParameter : public Parameter
  {
  public:

    /// Create bool-valued parameter
    BoolParameter(std::string key, bool value);

    /// Destructor
    ~BoolParameter();

    /// Assignment
    const BoolParameter& operator= (bool value);

    /// Cast parameter to bool
    operator bool() const;

    /// Return value type string
    std::string type_str() const;

    /// Return value string
    std::string value_str() const;

    /// Return range string
    std::string range_str() const;

    /// Return short string description
    std::string str() const;

  private:

    /// Parameter value
    bool _value;

  };

}

#endif
