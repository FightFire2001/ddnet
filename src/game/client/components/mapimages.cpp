/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/serverbrowser.h>
#include <game/client/component.h>
#include <engine/textrender.h>
#include <game/mapitems.h>

#include "mapimages.h"

CMapImages::CMapImages() : CMapImages(100)
{
}

CMapImages::CMapImages(int TextureSize) 
{
	m_Count = 0;
	m_TextureScale = TextureSize;
	mem_zero(m_EntitiesIsLoaded, sizeof(m_EntitiesIsLoaded));
	m_SpeedupArrowIsLoaded = false;

	mem_zero(m_aTextureUsedByTileOrQuadLayerFlag, sizeof(m_aTextureUsedByTileOrQuadLayerFlag));
}

void CMapImages::OnInit()
{
	InitOverlayTextures();
}

void CMapImages::OnMapLoadImpl(class CLayers *pLayers, IMap *pMap)
{
	// unload all textures
	for(int i = 0; i < m_Count; i++)
	{
		Graphics()->UnloadTexture(m_aTextures[i]);
		m_aTextures[i] = IGraphics::CTextureHandle();
		m_aTextureUsedByTileOrQuadLayerFlag[i] = 0;
	}
	m_Count = 0;

	int Start;
	pMap->GetType(MAPITEMTYPE_IMAGE, &Start, &m_Count);

	for(int g = 0; g < pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = pLayers->GetGroup(g);
		if(!pGroup)
		{
			continue;
		}

		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = pLayers->GetLayer(pGroup->m_StartLayer+l);
			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTLayer = (CMapItemLayerTilemap *)pLayer;
				if(pTLayer->m_Image != -1 && pTLayer->m_Image < (int)(sizeof(m_aTextures) / sizeof(m_aTextures[0])))
				{
					m_aTextureUsedByTileOrQuadLayerFlag[(size_t)pTLayer->m_Image] |= 1;
				}
			}
			else if(pLayer->m_Type == LAYERTYPE_QUADS)
			{
				CMapItemLayerQuads *pQLayer = (CMapItemLayerQuads *)pLayer;
				if(pQLayer->m_Image != -1 && pQLayer->m_Image < (int)(sizeof(m_aTextures) / sizeof(m_aTextures[0])))
				{
					m_aTextureUsedByTileOrQuadLayerFlag[(size_t)pQLayer->m_Image] |= 2;
				}
			}
		}
	}

	int TextureLoadFlag = Graphics()->HasTextureArrays() ? IGraphics::TEXLOAD_TO_2D_ARRAY_TEXTURE : IGraphics::TEXLOAD_TO_3D_TEXTURE;

	// load new textures
	for(int i = 0; i < m_Count; i++)
	{
		int LoadFlag = (((m_aTextureUsedByTileOrQuadLayerFlag[i] & 1) != 0) ? TextureLoadFlag : 0) | (((m_aTextureUsedByTileOrQuadLayerFlag[i] & 2) != 0) ? 0 : (Graphics()->IsTileBufferingEnabled() ? IGraphics::TEXLOAD_NO_2D_TEXTURE : 0));
		CMapItemImage *pImg = (CMapItemImage *)pMap->GetItem(Start+i, 0, 0);
		if(pImg->m_External)
		{
			char Buf[256];
			char *pName = (char *)pMap->GetData(pImg->m_ImageName);
			str_format(Buf, sizeof(Buf), "mapres/%s.png", pName);
			m_aTextures[i] = Graphics()->LoadTexture(Buf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, LoadFlag);
		}
		else
		{
			void *pData = pMap->GetData(pImg->m_ImageData);
			char *pName = (char *)pMap->GetData(pImg->m_ImageName);
			char aTexName[128];
			str_format(aTexName, sizeof(aTexName), "%s %s", "embedded:", pName);
			m_aTextures[i] = Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, CImageInfo::FORMAT_RGBA, pData, CImageInfo::FORMAT_RGBA, LoadFlag, aTexName);
			pMap->UnloadData(pImg->m_ImageData);
		}
	}
}

void CMapImages::OnMapLoad()
{
	IMap *pMap = Kernel()->RequestInterface<IMap>();
	CLayers *pLayers = m_pClient->Layers();
	OnMapLoadImpl(pLayers, pMap);
}

void CMapImages::LoadBackground(class CLayers *pLayers, class IMap *pMap)
{
	OnMapLoadImpl(pLayers, pMap);
}

bool CMapImages::HasFrontLayer()
{
	return GameClient()->m_GameInfo.m_EntitiesDDNet || GameClient()->m_GameInfo.m_EntitiesDDRace;
}

bool CMapImages::HasSpeedupLayer()
{
	return GameClient()->m_GameInfo.m_EntitiesDDNet || GameClient()->m_GameInfo.m_EntitiesDDRace;
}

bool CMapImages::HasSwitchLayer()
{
	return GameClient()->m_GameInfo.m_EntitiesDDNet || GameClient()->m_GameInfo.m_EntitiesDDRace;
}

bool CMapImages::HasTeleLayer()
{
	return GameClient()->m_GameInfo.m_EntitiesDDNet || GameClient()->m_GameInfo.m_EntitiesDDRace;
}

bool CMapImages::HasTuneLayer()
{
	return GameClient()->m_GameInfo.m_EntitiesDDNet || GameClient()->m_GameInfo.m_EntitiesDDRace;
}

IGraphics::CTextureHandle CMapImages::GetEntities(EMapImageEntityLayerType EntityLayerType)
{
	const char *pEntities = "ddnet";
	EMapImageModType EntitiesModType = MAP_IMAGE_MOD_TYPE_UNKNOWN;
	bool EntitesAreMasked = !GameClient()->m_GameInfo.m_DontMaskEntities;

	if(GameClient()->m_GameInfo.m_EntitiesDDNet && EntitesAreMasked)
	{
		pEntities = "ddnet";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_DDNET;
	}
	else if(GameClient()->m_GameInfo.m_EntitiesDDRace && EntitesAreMasked)
	{
		pEntities = "ddrace";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_DDRACE;
	}
	else if(GameClient()->m_GameInfo.m_EntitiesRace)
	{
		pEntities = "race";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_RACE;
	}
	else if (GameClient()->m_GameInfo.m_EntitiesBW)
	{
		pEntities = "blockworlds";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_BLOCKWORLDS;
	}
	else if(GameClient()->m_GameInfo.m_EntitiesFNG)
	{
		pEntities = "fng";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_FNG;
	}
	else if(GameClient()->m_GameInfo.m_EntitiesVanilla)
	{
		pEntities = "vanilla";
		EntitiesModType = MAP_IMAGE_MOD_TYPE_VANILLA;
	}

	if(!m_EntitiesIsLoaded[EntitiesModType])
	{
		m_EntitiesIsLoaded[EntitiesModType] = true;
		
		bool WasUnknwon = EntitiesModType == MAP_IMAGE_MOD_TYPE_UNKNOWN;

		char aPath[64];
		str_format(aPath, sizeof(aPath), "editor/entities_clear/%s.png", pEntities);

		bool GameTypeHasFrontLayer = HasFrontLayer() || WasUnknwon;
		bool GameTypeHasSpeedupLayer = HasSpeedupLayer() || WasUnknwon;
		bool GameTypeHasSwitchLayer = HasSwitchLayer() || WasUnknwon;
		bool GameTypeHasTeleLayer = HasTeleLayer() || WasUnknwon;
		bool GameTypeHasTuneLayer = HasTuneLayer() || WasUnknwon;

		int TextureLoadFlag = Graphics()->HasTextureArrays() ? IGraphics::TEXLOAD_TO_2D_ARRAY_TEXTURE : IGraphics::TEXLOAD_TO_3D_TEXTURE;

		CImageInfo ImgInfo;
		if(Graphics()->LoadPNG(&ImgInfo, aPath, IStorage::TYPE_ALL) && ImgInfo.m_Width > 0 && ImgInfo.m_Height > 0)
		{
			size_t ColorChannelCount = 0;
			if(ImgInfo.m_Format == CImageInfo::FORMAT_ALPHA)
				ColorChannelCount = 1;
			else if(ImgInfo.m_Format == CImageInfo::FORMAT_RGB)
				ColorChannelCount = 3;
			else if(ImgInfo.m_Format == CImageInfo::FORMAT_RGBA)
				ColorChannelCount = 4;

			size_t BuildImageSize = ColorChannelCount * (size_t)ImgInfo.m_Width * (size_t)ImgInfo.m_Height;

			uint8_t* pTmpImgData = (uint8_t*)ImgInfo.m_pData;
			uint8_t* pBuildImgData = (uint8_t*)malloc(BuildImageSize);
			
			// build game layer
			for(size_t n = 0; n < MAP_IMAGE_ENTITY_LAYER_TYPE_COUNT; ++n)
			{
				bool BuildThisLayer = true;
				if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_FRONT && !GameTypeHasFrontLayer)
					BuildThisLayer = false;
				else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_SPEEDUP && !GameTypeHasSpeedupLayer)
					BuildThisLayer = false;
				else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_SWITCH && !GameTypeHasSwitchLayer)
					BuildThisLayer = false;
				else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_TELE && !GameTypeHasTeleLayer)
					BuildThisLayer = false;
				else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_TUNE && !GameTypeHasTuneLayer)
					BuildThisLayer = false;

				dbg_assert(m_EntitiesTextures[EntitiesModType][n] == -1, "This should not happen.");

				if(BuildThisLayer)
				{
					// set everything transparent
					mem_zero(pBuildImgData, BuildImageSize);

					for(size_t i = 0; i < 256; ++i)
					{
						bool ValidTile = i != 0;
						size_t TileIndex = i;
						if(EntitiesModType == MAP_IMAGE_MOD_TYPE_DDNET || EntitiesModType == MAP_IMAGE_MOD_TYPE_DDRACE)
						{
							if(EntitiesModType == MAP_IMAGE_MOD_TYPE_DDNET || TileIndex != TILE_BOOST)
							{
								if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_GAME && !IsValidGameTile((int)TileIndex))
									ValidTile = false;
								else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_FRONT && !IsValidFrontTile((int)TileIndex))
									ValidTile = false;
								else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_SPEEDUP && !IsValidSpeedupTile((int)TileIndex))
									ValidTile = false;
								else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_SWITCH)
								{
									if(!IsValidSwitchTile((int)TileIndex))
										ValidTile = false;
									else
									{
										if(TileIndex == TILE_SWITCHTIMEDOPEN)
											TileIndex = 8;
									}								
								}
								else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_TELE && !IsValidTeleTile((int)TileIndex))
									ValidTile = false;
								else if(n == (size_t)MAP_IMAGE_ENTITY_LAYER_TYPE_TUNE && !IsValidTuneTile((int)TileIndex))
									ValidTile = false;
							}
						}
						else if((EntitiesModType == MAP_IMAGE_MOD_TYPE_RACE) && IsCreditsTile((int)TileIndex))
						{
							ValidTile = false;
						}
						else if((EntitiesModType == MAP_IMAGE_MOD_TYPE_FNG) && IsCreditsTile((int)TileIndex))
						{
							ValidTile = false;
						}
						else if((EntitiesModType == MAP_IMAGE_MOD_TYPE_VANILLA) && IsCreditsTile((int)TileIndex))
						{
							ValidTile = false;
						}
						/*else if((EntitiesModType == MAP_IMAGE_MOD_TYPE_RACE_BLOCKWORLD) && ...)
						{
							ValidTile = false;
						}*/

						size_t X = TileIndex % 16;
						size_t Y = TileIndex / 16;

						size_t CopyWidth = (size_t)ImgInfo.m_Width / 16;
						size_t CopyHeight = (size_t)ImgInfo.m_Height / 16;
						if(ValidTile)
						{
							Graphics()->CopyTextureBufferSub(pBuildImgData, pTmpImgData, (size_t)ImgInfo.m_Width, (size_t)ImgInfo.m_Height, ColorChannelCount, X * CopyWidth, Y * CopyHeight, CopyWidth, CopyHeight);
						}
					}

					m_EntitiesTextures[EntitiesModType][n] = Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format, pBuildImgData, ImgInfo.m_Format, TextureLoadFlag, aPath);
				}
				else
				{
					if(m_TransparentTexture == -1)
					{
						// set everything transparent
						mem_zero(pBuildImgData, BuildImageSize);

						m_TransparentTexture = Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format, pBuildImgData, ImgInfo.m_Format, TextureLoadFlag, aPath);
					}
					m_EntitiesTextures[EntitiesModType][n] = m_TransparentTexture;
				}				
			}

			free(pBuildImgData);
		}
	}

	return m_EntitiesTextures[EntitiesModType][(size_t)EntityLayerType];
}

IGraphics::CTextureHandle CMapImages::GetSpeedupArrow()
{
	if(!m_SpeedupArrowIsLoaded)
	{
		int TextureLoadFlag = (Graphics()->HasTextureArrays() ? IGraphics::TEXLOAD_TO_2D_ARRAY_TEXTURE_SINGLE_LAYER : IGraphics::TEXLOAD_TO_3D_TEXTURE_SINGLE_LAYER) | IGraphics::TEXLOAD_NO_2D_TEXTURE;
		m_SpeedupArrowTexture = Graphics()->LoadTexture(g_pData->m_aImages[IMAGE_SPEEDUP_ARROW].m_pFilename, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, TextureLoadFlag);

		m_SpeedupArrowIsLoaded = true;
	}

	return m_SpeedupArrowTexture;
}

IGraphics::CTextureHandle CMapImages::GetOverlayBottom()
{
	return m_OverlayBottomTexture;
}

IGraphics::CTextureHandle CMapImages::GetOverlayTop()
{
	return m_OverlayTopTexture;
}

IGraphics::CTextureHandle CMapImages::GetOverlayCenter()
{
	return m_OverlayCenterTexture;
}

void CMapImages::SetTextureScale(int Scale)
{
	if(m_TextureScale == Scale)
		return;

	m_TextureScale = Scale;

	if(Graphics() && m_OverlayCenterTexture != -1) // check if component was initialized
	{
		// reinitialize component
		Graphics()->UnloadTexture(m_OverlayBottomTexture);
		Graphics()->UnloadTexture(m_OverlayTopTexture);
		Graphics()->UnloadTexture(m_OverlayCenterTexture);

		m_OverlayBottomTexture = IGraphics::CTextureHandle();
		m_OverlayTopTexture = IGraphics::CTextureHandle();
		m_OverlayCenterTexture = IGraphics::CTextureHandle();

		InitOverlayTextures();
	}
}

int CMapImages::GetTextureScale()
{
	return m_TextureScale;
}

IGraphics::CTextureHandle CMapImages::UploadEntityLayerText(int TextureSize, int MaxWidth, int YOffset)
{	
	void *pMem = calloc(1024 * 1024 * 4, 1);

	UpdateEntityLayerText(pMem, 4, 1024, 1024, TextureSize, MaxWidth, YOffset, 0);
	UpdateEntityLayerText(pMem, 4, 1024, 1024, TextureSize, MaxWidth, YOffset, 1);
	UpdateEntityLayerText(pMem, 4, 1024, 1024, TextureSize, MaxWidth, YOffset, 2, 255);

	int TextureLoadFlag = (Graphics()->HasTextureArrays() ? IGraphics::TEXLOAD_TO_2D_ARRAY_TEXTURE : IGraphics::TEXLOAD_TO_3D_TEXTURE) | IGraphics::TEXLOAD_NO_2D_TEXTURE;
	IGraphics::CTextureHandle Texture = Graphics()->LoadTextureRaw(1024, 1024, CImageInfo::FORMAT_RGBA, pMem, CImageInfo::FORMAT_RGBA, TextureLoadFlag);
	free(pMem);

	return Texture;
}

void CMapImages::UpdateEntityLayerText(void* pTexBuffer, int ImageColorChannelCount, int TexWidth, int TexHeight, int TextureSize, int MaxWidth, int YOffset, int NumbersPower, int MaxNumber)
{
	char aBuf[4];
	int DigitsCount = NumbersPower+1;

	int CurrentNumber = pow(10, NumbersPower);
	
	if (MaxNumber == -1)
		MaxNumber = CurrentNumber*10-1;
	
	str_format(aBuf, 4, "%d", CurrentNumber);
	
	int CurrentNumberSuitableFontSize = TextRender()->AdjustFontSize(aBuf, DigitsCount, TextureSize, MaxWidth);
	int UniversalSuitableFontSize = CurrentNumberSuitableFontSize*0.95f; // should be smoothed enough to fit any digits combination

	int ApproximateTextWidth = TextRender()->CalculateTextWidth(aBuf, DigitsCount, 0, UniversalSuitableFontSize);
	int XOffSet = (64-ApproximateTextWidth)/2;
	YOffset += ((TextureSize - UniversalSuitableFontSize)/2);

	for (; CurrentNumber <= MaxNumber; ++CurrentNumber)
	{
		str_format(aBuf, 4, "%d", CurrentNumber);

		float x = (CurrentNumber%16)*64;
		float y = (CurrentNumber/16)*64;

		TextRender()->UploadEntityLayerText(pTexBuffer, ImageColorChannelCount, TexWidth, TexHeight, aBuf, DigitsCount, x+XOffSet, y+YOffset, UniversalSuitableFontSize);
	}
}

void CMapImages::InitOverlayTextures()
{
	int TextureSize = 64*m_TextureScale/100;
	int TextureToVerticalCenterOffset = (64-TextureSize)/2; // should be used to move texture to the center of 64 pixels area
	
	if(m_OverlayBottomTexture == -1)
	{
		m_OverlayBottomTexture = UploadEntityLayerText(TextureSize/2, 64, 32+TextureToVerticalCenterOffset/2);
	}

	if(m_OverlayTopTexture == -1)
	{
		m_OverlayTopTexture = UploadEntityLayerText(TextureSize/2, 64, TextureToVerticalCenterOffset/2);
	}

	if(m_OverlayCenterTexture == -1)
	{
		m_OverlayCenterTexture = UploadEntityLayerText(TextureSize, 64, TextureToVerticalCenterOffset);
	}
}
