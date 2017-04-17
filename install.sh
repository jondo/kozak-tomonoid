#! /bin/sh
# Run in terminal in this folder with command
# ./install.sh

PROJECT_FOLDER=Tomonoid
CMAKE_BF=build
NAME=tomonoid_generator
CMAKE_NAME=tomonoid
OUTPUT=output

if command -v cmake >/dev/null 2>&1; then
	echo "cmake found on system and will be used"
	cd $PROJECT_FOLDER
	if [ ! -d $CMAKE_BF ]; then
		mkdir build
	fi
	cd $CMAKE_BF
	
	if [ ! -d $OUTPUT ]; then
		mkdir $OUTPUT;
	fi	
	cmake ..
	make
	# for comfort copy tomonoid executive to top-level directory
	cd ../../
	if [ ! -d $OUTPUT ]; then
		mkdir $OUTPUT;
	fi
	cp $PROJECT_FOLDER/$CMAKE_BF/$CMAKE_NAME $NAME
else
	echo "cmake not found on system, backing_makefile will be used"
	cd $PROJECT_FOLDER
	make -f backing_makefile
	cp $NAME ../$NAME
	cd ..
	if [ ! -d $OUTPUT ]; then
		mkdir $OUTPUT;
	fi
fi



