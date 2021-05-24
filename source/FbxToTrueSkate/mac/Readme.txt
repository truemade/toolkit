***********************************
* FbxToTrueSkateCommunity for Mac *
***********************************

Prerequisites:
--------------

Xcode
https://apps.apple.com/au/app/xcode/id497799835?mt=12

FBX SDK 2015.1 Clang needs to be installed the the standard /Applications directory and can be obtained here...
https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2015


Building and Testing:
---------------------
- Ensure all prerequisites are met.
- Open the FbxToTrueSkate.xcodeproj found in mac/FbxToTrueSkate.
- Select the scheme you want to use (debug or release).
- Click Product -> Build or cmd + b
- From the Manage Scheme window in Xcode you can add command line arguments to test your build.
- Press Play, Product -> Run or cmd + r to run your build.


Packaging for Release:
----------------------
- Click Product -> Archive.
- After the archive is complete, the Archives window will appear.
- If Archives closes, you can reopen it with Window -> Organizer.
- Select the archive you just created.
- Click Distribute Content.
- Select Built Products.
- Choose a directory and file name.
- Export.
