#!/bin/bash

# Use this command-file to split image on channels
# call as: ./splitimage.sh image_pathname
# as result 4 files would be created in source image folder (r.png ... a.png)

ResourceEditorQt.app/Contents/MacOS/ResourceEditorQt -imagesplitter -split -file $1
