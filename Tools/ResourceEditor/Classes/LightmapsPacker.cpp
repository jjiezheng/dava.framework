#include "LightmapsPacker.h"

#include "Qt/Main/QtUtils.h"

LightmapsPacker::LightmapsPacker()
{
}

void LightmapsPacker::ParseSpriteDescriptors()
{
	FileList * fileList = new FileList(outputDir);

	char8 buf[512];
	uint32 readSize; 

	int32 itemsCount = fileList->GetCount();
	for(int32 i = 0; i < itemsCount; ++i)
	{
		const String & fileName = fileList->GetPathname(i);
		if(fileList->IsDirectory(i) || (FileSystem::Instance()->GetExtension(fileName) != ".txt"))
		{
			continue;
		}

		LightmapAtlasingData data;

		data.meshInstanceName = String(fileList->GetFilename(i), 0, fileList->GetFilename(i).length()-4);

		File * file = File::Create(fileName, File::OPEN | File::READ);
		
		file->ReadLine(buf, sizeof(buf)); //textures count

		readSize = file->ReadLine(buf, sizeof(buf)); //texture name
		String originalTextureName = outputDir + "/" + String(buf, readSize);
		data.textureName = originalTextureName;

		file->ReadLine(buf, sizeof(buf)); //image size

		file->ReadLine(buf, sizeof(buf)); //frames count

		file->ReadLine(buf, sizeof(buf)); //frame rect
		int32 x, y, dx, dy, unused0, unused1, unused2;
		sscanf(buf, "%d %d %d %d %d %d %d", &x, &y, &dx, &dy, &unused0, &unused1, &unused2);
		dx++;//cause TexturePacker::ReduceRectToOriginalSize removed one pixel by default
		dy++;

		Vector2 textureSize = GetTextureSize(originalTextureName);
		data.uvOffset = Vector2((float32)x/textureSize.x, (float32)y/textureSize.y);
		data.uvScale = Vector2((float32)dx/textureSize.x, (float32)dy/textureSize.y);
		
		file->Release();

		atlasingData.push_back(data);

		FileSystem::Instance()->DeleteFile(fileName);
	}

	fileList->Release();
}

Vector2 LightmapsPacker::GetTextureSize(const String & filePath)
{
	Vector2 ret;

	String sourceTexturePathname = FileSystem::Instance()->ReplaceExtension(filePath, TextureDescriptor::GetSourceTextureExtension());
    Image * image = CreateTopLevelImage(sourceTexturePathname);
    if(image)
    {
        ret.x = (float32)image->GetWidth();
        ret.y = (float32)image->GetHeight();
        
        SafeRelease(image);
    }

	return ret;
}

Vector<LightmapAtlasingData> * LightmapsPacker::GetAtlasingData()
{
	return &atlasingData;
}

void LightmapsPacker::Compress()
{
	FileList * fileList = new FileList(outputDir);

	int32 itemsCount = fileList->GetCount();
	for(int32 i = 0; i < itemsCount; ++i)
	{
		const String & fileName = fileList->GetPathname(i);
		if(fileList->IsDirectory(i) || (FileSystem::Instance()->GetExtension(fileName) != ".png"))
		{
			continue;
		}

		TextureDescriptor descriptor;
        descriptor.Save(TextureDescriptor::GetDescriptorPathname(fileName));
	}

	fileList->Release();
}
