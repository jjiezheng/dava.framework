#include "TextureListModel.h"
#include "Scene/SceneDataManager.h"
#include <QPainter>
#include <QFileInfo>

TextureListModel::TextureListModel(QObject *parent /* = 0 */) 
	: QAbstractListModel(parent)
	, curSortMode(TextureListModel::SortByName)
	, curFilterBySelectedNode(false)
{}

TextureListModel::~TextureListModel()
{
	clear();
}

int TextureListModel::rowCount(const QModelIndex & /* parent */) const
{
	return textureDescriptorsFiltredSorted.size();
}

QVariant TextureListModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid())
	{
		const DAVA::TextureDescriptor *curTextureDescriptor = textureDescriptorsFiltredSorted[index.row()];

		switch(role)
		{
		case Qt::DisplayRole:
			return QVariant(QFileInfo(curTextureDescriptor->GetSourceTexturePathname().c_str()).fileName());
			break;

		default:
			break;
		}
	}

	return QVariant();
}

DAVA::Texture* TextureListModel::getTexture(const QModelIndex &index) const
{
	DAVA::Texture *ret = NULL;
	DAVA::TextureDescriptor *desc = getDescriptor(index);

	if(index.isValid() && texturesAll.contains(desc))
	{
		ret = texturesAll[desc];
	}

	return ret;
}

DAVA::Texture* TextureListModel::getTexture(const DAVA::TextureDescriptor* descriptor) const
{
	DAVA::Texture *ret = NULL;

	if(texturesAll.contains(descriptor))
	{
		ret = texturesAll[descriptor];
	}

	return ret;
}

DAVA::TextureDescriptor* TextureListModel::getDescriptor(const QModelIndex &index) const
{
	DAVA::TextureDescriptor *ret = NULL;

	if(index.isValid() && textureDescriptorsFiltredSorted.size() > index.row())
	{
		ret = textureDescriptorsFiltredSorted[index.row()];
	}

	return ret;
}

void TextureListModel::setTexture(const DAVA::TextureDescriptor* descriptor, DAVA::Texture *texture)
{
	if(texturesAll.contains(descriptor))
	{
		texturesAll[descriptor] = texture;
	}
}

bool TextureListModel::isHighlited(const QModelIndex &index) const
{
	bool ret = false;
	DAVA::TextureDescriptor *descriptor = getDescriptor(index);

	if(NULL != descriptor)
	{
		ret = textureDescriptorsHighlight.contains(descriptor);
	}

	return ret;
}

void TextureListModel::dataReady(const DAVA::TextureDescriptor *desc)
{
	int i = textureDescriptorsFiltredSorted.indexOf((DAVA::TextureDescriptor * const) desc);
	emit dataChanged(this->index(i), this->index(i));
}

void TextureListModel::setFilter(QString filter)
{
	beginResetModel();
	curFilter = filter;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setFilterBySelectedNode(bool enabled)
{
	beginResetModel();
	curFilterBySelectedNode = enabled;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setSortMode(TextureListModel::TextureListSortMode sortMode)
{
	beginResetModel();
	curSortMode = sortMode;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setScene(DAVA::Scene *scene)
{
	beginResetModel();

	clear();

	DAVA::Map<DAVA::String, DAVA::Texture *> texturesInNode;
	SceneDataManager::EnumerateTextures(scene, texturesInNode);

	for(DAVA::Map<DAVA::String, DAVA::Texture *>::iterator t = texturesInNode.begin(); t != texturesInNode.end(); ++t)
	{
		const DAVA::String descPath = t->first;

		// if there is no the same descriptor and this file exists
		if(DAVA::FileSystem::Instance()->IsFile(descPath))
		{
			DAVA::TextureDescriptor * descriptor = DAVA::TextureDescriptor::CreateFromFile(descPath);

			if(NULL != descriptor)
			{
				textureDescriptorsAll.push_back(descriptor);
				texturesAll[descriptor] = t->second;
			}
		}
	}

	applyFilterAndSort();

	endResetModel();
}

void TextureListModel::setHighlight(DAVA::Entity *node)
{
	beginResetModel();

	textureDescriptorsHighlight.clear();

	DAVA::Map<DAVA::String, DAVA::Texture *> texturesInNode;
	SceneDataManager::EnumerateTextures(node, texturesInNode);

	for(DAVA::Map<DAVA::String, DAVA::Texture *>::iterator t = texturesInNode.begin(); t != texturesInNode.end(); ++t)
	{
		const DAVA::String descPath = t->first;
		for(int i = 0; i < textureDescriptorsAll.size(); ++i)
		{
			if(textureDescriptorsAll[i]->pathname == descPath)
			{
				textureDescriptorsHighlight.push_back(textureDescriptorsAll[i]);
			}
		}
	}

	if(curFilterBySelectedNode)
	{
		applyFilterAndSort();
	}

	endResetModel();
}

void TextureListModel::clear()
{
	texturesAll.clear();
	textureDescriptorsHighlight.clear();
	textureDescriptorsFiltredSorted.clear();

	for(int i = 0; i < textureDescriptorsAll.size(); ++i)
	{
		DAVA::SafeRelease(textureDescriptorsAll[i]);
	}

	textureDescriptorsAll.clear();
}

void TextureListModel::applyFilterAndSort()
{
	textureDescriptorsFiltredSorted.clear();

	for(int i = 0; i < (int) textureDescriptorsAll.size(); ++i)
	{
		if( (curFilter.isEmpty() || DAVA::String::npos != textureDescriptorsAll[i]->pathname.find(curFilter.toStdString())) &&	// text filter
			(!curFilterBySelectedNode || textureDescriptorsHighlight.contains(textureDescriptorsAll[i])))						// cur selected node filter
		{
			textureDescriptorsFiltredSorted.push_back(textureDescriptorsAll[i]);
		}
	}

	switch(curSortMode)
	{
	case SortByName:
		{
			SortFnByName fn;
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case SortByFileSize:
		{
			SortFnByFileSize fn;
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case  SortByImageSize:
		{
			SortFnByImageSize fn(this);
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case  SortByDataSize:
		{
			SortFnByDataSize fn(this);
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	default:
		break;
	}
}

bool SortFnByName::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	return QFileInfo(t1->pathname.c_str()).completeBaseName() < QFileInfo(t2->pathname.c_str()).completeBaseName();
}

bool SortFnByFileSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	return QFileInfo(t1->GetSourceTexturePathname().c_str()).size() < QFileInfo(t2->GetSourceTexturePathname().c_str()).size();
}

bool SortFnByImageSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	DAVA::Texture *tx1 = model->getTexture(t1);
	DAVA::Texture *tx2 = model->getTexture(t2);

	return (tx1->width * tx1->height) < (tx2->width * tx2->height);
}

bool SortFnByDataSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	DAVA::Texture *tx1 = model->getTexture(t1);
	DAVA::Texture *tx2 = model->getTexture(t2);

	return (tx1->width * tx1->height * DAVA::Texture::GetPixelFormatSizeInBytes(tx1->format)) < (tx2->width * tx2->height * DAVA::Texture::GetPixelFormatSizeInBytes(tx2->format));
}
