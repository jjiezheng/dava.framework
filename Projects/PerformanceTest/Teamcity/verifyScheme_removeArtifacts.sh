# !/bin/bash

DIR_SCH="xcuserdata/$(whoami).xcuserdatad/xcschemes"

#verify schemes
if [ -f "../TemplateProjectMacOS.xcodeproj/$DIR_SCH/TemplateProjectMacOS.xcscheme" ]; then
   echo "scheme for MacOS exists"
else
  mkdir -p "../TemplateProjectMacOS.xcodeproj/$DIR_SCH/"
  cp -Rf schemes/MacOS/* "../TemplateProjectMacOS.xcodeproj/$DIR_SCH" 
fi

if [ -f "../TemplateProjectiPhone.xcodeproj/$DIR_SCH/PerformanceTest.xcscheme" ]; then
   echo "scheme for iPhone exists"
else
  mkdir -p "../TemplateProjectiPhone.xcodeproj/$DIR_SCH/"
  cp -Rf schemes/iPhone/* "../TemplateProjectiPhone.xcodeproj/$DIR_SCH" 
fi

#remove artifacts
if [ -f "../Reports/report.html" ]; then
   echo "artifacts exist"
   rm -rf ../Reports/report.html
   rm -rf ../DerivedData/TemplateProjectiPhone/Build/Products/AdHoc-iphoneos/PerformanceTest.ipa
   echo "artifacts were deleted"
else
  echo "artifacts don't exist"
fi