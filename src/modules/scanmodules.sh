#!/bin/sh
#
# This script scans subdirectories for files <modulename>.module
# and creates the file modules.list and dolfin_modules.h

MODULES="modules.list"
HEADER="../kernel/main/dolfin_modules.h"

rm -f $MODULES
rm -f $HEADER

# Prepare file modules.list
echo "# This file is automatically generated by the script scanmodules.sh" > $MODULES

# Find all modules
DIRS=`ls`
COUNT="0"
for d in $DIRS; do
	 if [ -r $d/$d.module ]; then
		  echo $d >> $MODULES
        COUNT=`echo $COUNT | awk '{ print $1+1 }'`
    fi
done
echo "Found $COUNT modules in src/modules:"
echo ""

# Generate code
echo "// This code is automatically generated by the script scanmodules.sh" >> $HEADER
echo "// Modules are automatically detected from src/modules/ as directories" >> $HEADER
echo "// containing a file src/modules/<name>/<name>.module." >> $HEADER
echo " " >> $HEADER
echo "#include <dolfin/Solver.h>" >> $HEADER

# Include files
for d in `cat $MODULES | grep -v '#'`; do
	
	# Find NAME from config file
	NAME=`cat $d/$d.module | grep NAME | cut -d'"' -f2`
	if [ 'x'$NAME = 'x' ]; then
		echo "Unable to find keyword NAME for module $d in file src/modules/$d/$d.module"
		exit 1
	fi
	if [ $NAME != $d ]; then
		echo "Module name $NAME does not match name of directory $d"
		exit 1
	fi
	
	# Find KEYWORD from config file (can contain more than one word!)
	KEYWORD=`cat $d/$d.module | grep KEYWORD | cut -d'"' -f2`
	CHECK=`echo $KEYWORD | awk '{ print $1 }'`
	if [ 'x'$CHECK = 'x' ]; then
		echo "Unable to find keyword KEYWORD for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Find SOLVER from config file
	SOLVER=`cat $d/$d.module | grep SOLVER | cut -d'"' -f2`
	if [ 'x'$SOLVER = 'x' ]; then
		echo "Unable to find keyword SOLVER for module $d in file src/modules/$d/$d.module"
		exit 1
 	fi

	# Find SETTINGS from config file
	SETTINGS=`cat $d/$d.module | grep SETTINGS | cut -d'"' -f2`
	if [ 'x'$SETTINGS = 'x' ]; then
		echo "Unable to find keyword SETTINGS for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Write a nice message
	echo "    - $NAME ($KEYWORD)"
	 
	# Include files
	echo "#include \"$SOLVER.h\"" >> $HEADER
	echo "#include \"$SETTINGS.h\"" >> $HEADER
done

echo "" >> $HEADER
echo "#include <string.h>" >> $HEADER
echo "" >> $HEADER
echo "#define DOLFIN_MODULE_COUNT $COUNT" >> $HEADER
echo "" >> $HEADER

# Generate function dolfin_module_solver(const char * problem)
echo "dolfin::Solver * dolfin_module_solver(const char *problem)" >> $HEADER
echo "{" >> $HEADER
for d in `cat $MODULES | grep -v '#'`; do

	# Find KEYWORD from config file
	KEYWORD=`cat $d/$d.module | grep KEYWORD | cut -d'"' -f2`
	CHECK=`echo $KEYWORD | awk '{ print $1 }'`
	if [ 'x'$CHECK = 'x' ]; then
		echo "Unable to find keyword KEYWORD for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Find SOLVER from config file
	SOLVER=`cat $d/$d.module | grep SOLVER | cut -d'"' -f2`
	if [ 'x'$SOLVER = 'x' ]; then
		echo "Unable to find keyword SOLVER for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Find GRID from config file
	GRID=`cat $d/$d.module | grep GRID | cut -d'"' -f2`
	if [ 'x'$GRID = 'x' ]; then
		echo "Unable to find keyword GRID for module $d in file src/modules/$d/$d.module"
		exit 1
	fi
		
	# Include only modules that don't want to use a grid
	if [ $GRID = 'yes' ]; then
		echo "    if ( strcasecmp(problem,\"$KEYWORD\") == 0 ) {" >> $HEADER
		echo "        cout << \"Solver for problem \\\"\" << problem << \"\\\" needs a grid.\" << endl;" >> $HEADER
		echo "    }" >> $HEADER
	else
		echo "    if ( strcasecmp(problem,\"$KEYWORD\") == 0 )" >> $HEADER
		echo "        return new dolfin::$SOLVER();" >> $HEADER
	fi

done

echo "" >> $HEADER
echo "    cout << \"Could not find any matching solver for problem \\\"\" << problem << \"\\\"\" << endl;" >> $HEADER
echo "" >> $HEADER
echo "    return 0;" >> $HEADER
echo "}" >> $HEADER
echo "" >> $HEADER

# Generate function dolfin_module_solver(const char *problem, Grid &grid)
echo "dolfin::Solver * dolfin_module_solver(const char *problem, dolfin::Grid &grid)" >> $HEADER
echo "{" >> $HEADER
for d in `cat $MODULES | grep -v '#'`; do

	# Find KEYWORD from config file
	KEYWORD=`cat $d/$d.module | grep KEYWORD | cut -d'"' -f2`
	CHECK=`echo $KEYWORD | awk '{ print $1 }'`
	if [ 'x'$CHECK = 'x' ]; then
		echo "Unable to find keyword KEYWORD for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Find SOLVER from config file
	SOLVER=`cat $d/$d.module | grep SOLVER | cut -d'"' -f2`
	if [ 'x'$SOLVER = 'x' ]; then
		echo "Unable to find keyword SOLVER for module $d in file src/modules/$d/$d.module"
		exit 1
	fi
	
	# Include only modules that want to use a grid
	if [ $GRID = 'yes' ]; then
		echo "    if ( strcasecmp(problem,\"$KEYWORD\") == 0 )" >> $HEADER
		echo "        return new dolfin::$SOLVER(grid);" >> $HEADER
	else
		echo "    if ( strcasecmp(problem,\"$KEYWORD\") == 0 )" >> $HEADER
		echo "        cout << \"Solver for problem \\\"\" << problem << \"\\\" needs a grid.\" << endl;" >> $HEADER
	fi

done

echo "" >> $HEADER
echo "    cout << \"Could not find any matching solver for problem \\\"\" << problem << \"\\\"\" << endl;" >> $HEADER
echo "" >> $HEADER
echo "    return 0;" >> $HEADER
echo "}" >> $HEADER
echo "" >> $HEADER

# Generate function dolfin_module_settings
echo "void dolfin_module_init_settings(const char *problem)" >> $HEADER
echo "{" >> $HEADER
for d in `cat $MODULES | grep -v '#'`; do
 
	# Find KEYWORD from config file
	KEYWORD=`cat $d/$d.module | grep KEYWORD | cut -d'"' -f2`
	CHECK=`echo $KEYWORD | awk '{ print $1 }'`
	if [ 'x'$CHECK = 'x' ]; then
		echo "Unable to find keyword KEYWORD for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	# Find SETTINGS from config file
	SETTINGS=`cat $d/$d.module | grep SETTINGS | cut -d'"' -f2`
	if [ 'x'$SETTINGS = 'x' ]; then
		echo "Unable to find keyword SETTINGS for module $d in file src/modules/$d/$d.module"
		exit 1
	fi

	echo "    if ( strcasecmp(problem,\"$KEYWORD\") == 0 ) {" >> $HEADER
	echo "        dolfin::$SETTINGS settings;" >> $HEADER
   echo "        return;" >> $HEADER
   echo "    }" >> $HEADER
done

echo "" >> $HEADER
echo "    cout << \"Could not find any matching settings for problem \\\"\" << problem << \"\\\"\" << endl;" >> $HEADER

echo "" >> $HEADER
echo "}" >> $HEADER

echo " "
