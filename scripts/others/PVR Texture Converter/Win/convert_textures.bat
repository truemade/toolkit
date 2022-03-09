@echo off

IF NOT EXIST ".\exported" md ".\exported"

echo "Converting images to PVR format for iOS"
FOR %%f IN (*.png *.jpg *.jpeg) DO ( 
	START C:\"Imagination Technologies"\PowerVR_Graphics\PowerVR_Tools\PVRTexTool\CLI\Windows_x86_64\PVRTexToolCLI.exe -m -e pvrtchigh -f PVRTCI_4BPP_RGBA,UBN,sRGB -legacypvr -i %%f -o .\exported\%%~nf.pvr
)

echo "Converting images to PVR format for Android"
FOR %%f IN (*.png *.jpg *.jpeg) DO ( 
	START C:\"Imagination Technologies"\PowerVR_Graphics\PowerVR_Tools\PVRTexTool\CLI\Windows_x86_64\PVRTexToolCLI.exe -m -e etcslow -f ETC1,UB,sRGB -legacypvr -i %%f -o .\exported\%%~nf_etc1.pvr
)

pause