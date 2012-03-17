#include "LightmapsPacker.h"
#include "ResourcePackerScreen.h"
#include "../TexturePacker/CommandLineParser.h"

LightmapsPacker::LightmapsPacker()
{

}

LightmapsPacker::~LightmapsPacker()
{

}

void LightmapsPacker::SetInputDir(const String & _inputDir)
{
	inputDir = _inputDir;
}

void LightmapsPacker::SetOutputDir(const String & _outputDir)
{
	outputDir = _outputDir;
}

void LightmapsPacker::Pack()
{
	FileSystem::Instance()->CreateDirectory(outputDir, false);

	ResourcePackerScreen * resourcePackerScreen = new ResourcePackerScreen();

	CommandLineParser::Instance()->ClearFlags();

	resourcePackerScreen->inputGfxDirectory = inputDir;
	resourcePackerScreen->outputGfxDirectory = outputDir;
	resourcePackerScreen->excludeDirectory = inputDir + "/../";
	resourcePackerScreen->processAllPng = true;

	resourcePackerScreen->PackResources();

	SafeRelease(resourcePackerScreen);
}

void LightmapsPacker::Compress()
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

		File * file = File::Create(fileName, File::OPEN | File::READ);
		
		file->ReadLine(buf, sizeof(buf)); //textures count

		readSize = file->ReadLine(buf, sizeof(buf)); //texture name
		data.textureName = String(buf, readSize);

		file->ReadLine(buf, sizeof(buf)); //image size

		file->ReadLine(buf, sizeof(buf)); //frames count

		file->ReadLine(buf, sizeof(buf)); //frame rect
		int32 x, y, dx, dy, unused0, unused1, unused2;
		sscanf(buf, "%d %d %d %d %d %d %d", &x, &y, &dx, &dy, &unused0, &unused1, &unused2);
		dx++;//cause TexturePacker::ReduceRectToOriginalSize removed one pixel by default
		dy++;

		data.uvOffset = Vector2((float32)x, (float32)y);

		Vector2 textureSize = GetTextureSize(outputDir+"/"+data.textureName);
		data.uvScale = Vector2((float32)dx/textureSize.x, (float32)dy/textureSize.y);
		
		file->Release();

		lightmapsData.push_back(data);
	}

	fileList->Release();
}

Vector2 LightmapsPacker::GetTextureSize(const String & filePath)
{
	Vector2 ret;

	Image * image = new Image();
	if(1 != LibPngWrapper::ReadPngFile(filePath.c_str(), image))
	{
		SafeRelease(image);
		return ret;
	}

	ret.x = (float32)image->GetWidth();
	ret.y = (float32)image->GetHeight();

	SafeRelease(image);

	return ret;
}