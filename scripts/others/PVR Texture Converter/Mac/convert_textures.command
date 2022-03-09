#!/bin/bash
cd `dirname $0`

mkdir exported

for file in *.png *.jpg *.jpeg *.PNG *.JPG *.JPEG
do
	inputName="${file}"
	noExtension=$(echo "$inputName" | cut -d'/' -f 2 | cut -d'.' -f 1)

	# Use this line to force lowercase
	#outputName=$(echo exported/${noExtension}.pvr | awk '{print tolower($0)}')
	outputName="$(echo exported/${noExtension}.pvr)"
	echo "Converting for iOS $inputName to $outputName"
	bin/texturetool -m -e PVRTC -o "$outputName" -f PVR "$inputName"

	# Use this line to force lowercase
	#outputName=$(echo exported/${noExtension}_etc1.pvr | awk '{print tolower($0)}')
	outputName="$(echo exported/${noExtension}_etc1.pvr)"
	echo "Converting for Android $inputName to $outputName"
	bin/PVRTexToolCLI_mac -f ETC1 -q etcslow -m -legacypvr -o "${outputName}" -i "${file}"
done

