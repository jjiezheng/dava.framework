#include "ImageSplitter.h"
#include "../Qt/Main/QtUtils.h"

using namespace DAVA;


bool ImageSplitter::SplitImage(const String &pathname, Set<String> &errorLog)
{
    Image *loadedImage = CreateTopLevelImage(pathname);
    if(!loadedImage)
    {
        errorLog.insert(String(Format("Can't load image %s", pathname.c_str())));
        return false;
    }
    
    if(loadedImage->GetPixelFormat() != FORMAT_RGBA8888)
    {
        errorLog.insert(String(Format("Incorrect image format %s. Must be RGBA8888", Texture::GetPixelFormatString(loadedImage->GetPixelFormat()))));
        return false;
    }
    

    Image *red = Image::Create(loadedImage->width, loadedImage->height, FORMAT_A8);
    Image *green = Image::Create(loadedImage->width, loadedImage->height, FORMAT_A8);
    Image *blue = Image::Create(loadedImage->width, loadedImage->height, FORMAT_A8);
    Image *alpha = Image::Create(loadedImage->width, loadedImage->height, FORMAT_A8);

    int32 size = loadedImage->width * loadedImage->height;
    int32 pixelSize = Texture::GetPixelFormatSizeInBytes(FORMAT_RGBA8888);
    for(int32 i = 0; i < size; ++i)
    {
        int32 offset = i * pixelSize;
        red->data[i] = loadedImage->data[offset];
        green->data[i] = loadedImage->data[offset + 1];
        blue->data[i] = loadedImage->data[offset + 2];
        alpha->data[i] = loadedImage->data[offset + 3];
    }
    
    String folder, filename;
    FileSystem::Instance()->SplitPath(pathname, folder, filename);
    
    SaveImage(red, folder + "/r.png");
    SaveImage(green, folder + "/g.png");
    SaveImage(blue, folder + "/b.png");
    SaveImage(alpha, folder + "/a.png");


    ReleaseImages(red, green, blue, alpha);
    SafeRelease(loadedImage);
    return true;
}

bool ImageSplitter::MergeImages(const String &folder, Set<String> &errorLog)
{
    Image *red = LoadImage(folder + "/r.png");
    Image *green = LoadImage(folder + "/g.png");
    Image *blue = LoadImage(folder + "/b.png");
    Image *alpha = LoadImage(folder + "/a.png");

    if(!red || !green || !blue || !alpha)
    {
        errorLog.insert(String(Format("Can't load one or more channel images from folder %s", folder.c_str())));
        ReleaseImages(red, green, blue, alpha);
        return false;
    }
    
    if(red->GetPixelFormat() != FORMAT_A8 || green->GetPixelFormat() != FORMAT_A8 || blue->GetPixelFormat() != FORMAT_A8 || alpha->GetPixelFormat() != FORMAT_A8)
    {
        errorLog.insert(String("Can't merge images. Source format must be Grayscale 8bit"));
        ReleaseImages(red, green, blue, alpha);
        return false;
    }
    
    if(     (red->width != green->width || red->width != blue->width || red->width != alpha->width)
       ||   (red->height != green->height || red->height != blue->height || red->height != alpha->height))
    {
        errorLog.insert(String("Can't merge images. Source images must have same size"));
        ReleaseImages(red, green, blue, alpha);
        return false;
    }

    Image *mergedImage = Image::Create(red->width, red->height, FORMAT_RGBA8888);
    int32 size = mergedImage->width * mergedImage->height;
    int32 pixelSize = Texture::GetPixelFormatSizeInBytes(FORMAT_RGBA8888);
    for(int32 i = 0; i < size; ++i)
    {
        int32 offset = i * pixelSize;
        mergedImage->data[offset] = red->data[i];
        mergedImage->data[offset + 1] = green->data[i];
        mergedImage->data[offset + 2] = blue->data[i];
        mergedImage->data[offset + 3] = alpha->data[i];
    }

    ImageLoader::Save(mergedImage, FileSystem::Instance()->GetCanonicalPath(folder + "/merged.png"));
    
    ReleaseImages(red, green, blue, alpha);
    SafeRelease(mergedImage);
    return true;
}

void ImageSplitter::SaveImage(Image *image, const String &pathname)
{
    ImageLoader::Save(image, FileSystem::Instance()->GetCanonicalPath(pathname));
}

Image * ImageSplitter::LoadImage(const String &pathname)
{
    return CreateTopLevelImage(FileSystem::Instance()->GetCanonicalPath(pathname));
}

void ImageSplitter::ReleaseImages(Image *r, Image *g, Image *b, Image *a)
{
    SafeRelease(r);
    SafeRelease(g);
    SafeRelease(b);
    SafeRelease(a);
}


