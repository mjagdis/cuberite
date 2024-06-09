
// MapSerializer.cpp


#include "Globals.h"
#include "MapSerializer.h"
#include "OSSupport/GZipFile.h"
#include "FastNBT.h"

#include "../Map.h"
#include "../World.h"





cMapSerializer::cMapSerializer(const AString & a_WorldName, cMap * a_Map):
	m_Map(a_Map)
{
	auto DataPath = fmt::format(FMT_STRING("{}{}data"), a_WorldName, cFile::PathSeparator());
	m_Path = fmt::format(FMT_STRING("{}{}map_{}.dat"), DataPath, cFile::PathSeparator(), a_Map->GetID());
	cFile::CreateFolder(DataPath);
}





bool cMapSerializer::Load()
{
	const auto Data = GZipFile::ReadRestOfFile(m_Path);
	const cParsedNBT NBT(Data.GetView());

	if (!NBT.IsValid())
	{
		// NBT Parsing failed
		return false;
	}

	return LoadMapFromNBT(NBT);
}





bool cMapSerializer::Save(void)
{
	cFastNBTWriter Writer;
	SaveMapToNBT(Writer);
	Writer.Finish();

	#ifndef NDEBUG
	cParsedNBT TestParse(Writer.GetResult());
	ASSERT(TestParse.IsValid());
	#endif  // !NDEBUG

	GZipFile::Write(m_Path, Writer.GetResult());

	return true;
}





void cMapSerializer::SaveMapToNBT(cFastNBTWriter & a_Writer)
{
	a_Writer.BeginCompound("data");

	a_Writer.AddByte("scale", static_cast<Byte>(m_Map->GetScale()));
	a_Writer.AddByte("dimension", static_cast<Byte>(m_Map->GetDimension()));

	// 1.12 and earlier includes width and height, later don't
	a_Writer.AddShort("width",  static_cast<Int16>(m_Map->MAP_WIDTH));
	a_Writer.AddShort("height", static_cast<Int16>(m_Map->MAP_HEIGHT));

	// 1.14 and later include locked
	a_Writer.AddByte("locked", (m_Map->m_Locked ? 1 : 0));

	// 1.12 and later include trackingPosition and unlimitedTracking
	a_Writer.AddByte("trackingPosition", (m_Map->m_TrackingPosition ? 1 : 0));
	a_Writer.AddByte("unlimitedTracking", (m_Map->m_UnlimitedTracking ? 1 : 0));

	// Configurable tracking thresholds are a Cuberite addition (they're hardcoded in Minecraft)
	a_Writer.AddInt("TrackingThreshold", m_Map->GetTrackingThreshold());
	a_Writer.AddInt("FarTrackingThreshold", m_Map->GetFarTrackingThreshold());

	a_Writer.AddInt("xCenter", m_Map->GetCenterX());
	a_Writer.AddInt("zCenter", m_Map->GetCenterZ());

	// Minecraft Java prior to 1.13 does not save decorations to the map files.
	// From 1.13 onwards it writes separate "frames" and "banners" lists each with
	// slightly different formats.
	a_Writer.BeginList("frames", TAG_Compound);
	for (const auto & itr : m_Map->GetDecorators())
	{
		if (itr.first.m_Type != cMap::DecoratorType::PLAYER)
		{
			a_Writer.BeginCompound("");

			a_Writer.AddInt("EntityId", itr.first.m_Id);
			a_Writer.AddInt("Rotation", itr.second.m_Yaw);

			a_Writer.BeginCompound("Pos");
				a_Writer.AddInt("X", itr.second.m_Position.x);
				a_Writer.AddInt("Y", itr.second.m_Position.y);
				a_Writer.AddInt("Z", itr.second.m_Position.z);
			a_Writer.EndCompound();

			// Cuberite also has non-item frame decorators that may have been added
			// by plugins. We need to save type and icon for those.
			a_Writer.AddByte("Type", static_cast<unsigned char>(itr.first.m_Type));
			a_Writer.AddByte("Icon", static_cast<unsigned char>(itr.second.m_Icon));

			a_Writer.EndCompound();
		}
	}
	a_Writer.EndList();

	a_Writer.AddByteArray("colors", reinterpret_cast<const char *>(m_Map->m_Data), m_Map->GetNumPixels());

	a_Writer.EndCompound();
}





bool cMapSerializer::LoadMapFromNBT(const cParsedNBT & a_NBT)
{
	int Data = a_NBT.FindChildByName(0, "data");
	if (Data < 0)
	{
		return false;
	}

	int CurrLine = a_NBT.FindChildByName(Data, "scale");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		unsigned int Scale = static_cast<unsigned int>(a_NBT.GetByte(CurrLine));
		m_Map->SetScale(Scale);
	}

	CurrLine = a_NBT.FindChildByName(Data, "dimension");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		eDimension Dimension = static_cast<eDimension>(a_NBT.GetByte(CurrLine));

		if (Dimension != m_Map->m_World->GetDimension())
		{
			// TODO 2014-03-20 xdot: We should store nether maps in nether worlds, e.t.c.
			return false;
		}
	}

	// 1.14 and later include locked
	CurrLine = a_NBT.FindChildByName(Data, "locked");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		m_Map->m_Locked = a_NBT.GetByte(CurrLine);
	}

	// 1.12 and later include trackingPosition and unlimitedTracking
	CurrLine = a_NBT.FindChildByName(Data, "trackingPosition");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		m_Map->m_TrackingPosition = a_NBT.GetByte(CurrLine);
	}
	CurrLine = a_NBT.FindChildByName(Data, "unlimitedTracking");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		m_Map->m_UnlimitedTracking = a_NBT.GetByte(CurrLine);
	}

	// Configurable tracking thresholds are a Cuberite addition (they're hardcoded in Minecraft)
	CurrLine = a_NBT.FindChildByName(Data, "TrackingThreshold");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
	{
		m_Map->m_TrackingThreshold = a_NBT.GetInt(CurrLine);
	}
	CurrLine = a_NBT.FindChildByName(Data, "FarTrackingThreshold");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
	{
		m_Map->m_FarTrackingThreshold = a_NBT.GetInt(CurrLine);
	}

	CurrLine = a_NBT.FindChildByName(Data, "xCenter");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
	{
		int CenterX = a_NBT.GetInt(CurrLine);
		m_Map->m_CenterX = CenterX;
	}

	CurrLine = a_NBT.FindChildByName(Data, "zCenter");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
	{
		int CenterZ = a_NBT.GetInt(CurrLine);
		m_Map->m_CenterZ = CenterZ;
	}

	// Only Minecraft Java from 1.13 onwards has decorations in the map file. With map files
	// from earlier versions frames will only appear after the chunk they are in has loaded
	// and they are spawned.
	int Frames = a_NBT.FindChildByName(Data, "frames");
	if ((Frames >= 0) && (a_NBT.GetType(Frames) == TAG_List))
	{
		for (int Frame = a_NBT.GetFirstChild(Frames); Frame >= 0; Frame = a_NBT.GetNextSibling(Frame))
		{
			cMap::DecoratorType Type = cMap::DecoratorType::FRAME;
			cMap::icon Icon = cMap::icon::MAP_ICON_GREEN_ARROW;
			Vector3d Position;
			int EntityId = 0, Yaw = 0;

			// The decoration structure changed somewhat arbitrarily (IMHO) in 1.20.
			// Cuberite currently saves using the pre-1.20 style.
			CurrLine = a_NBT.FindChildByName(Frame, "EntityId");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
			{
				// Pre-1.20
				EntityId = a_NBT.GetInt(CurrLine);

				CurrLine = a_NBT.FindChildByName(Frame, "Rotation");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
				{
					Yaw = a_NBT.GetInt(CurrLine);
				}

				int Pos = a_NBT.FindChildByName(Frame, "Pos");
				if ((Pos >= 0) && (a_NBT.GetType(Pos) == TAG_Compound))
				{
					CurrLine = a_NBT.FindChildByName(Pos, "X");
					if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
					{
						Position.x = a_NBT.GetInt(CurrLine);
					}

					CurrLine = a_NBT.FindChildByName(Pos, "Y");
					if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
					{
						Position.y = a_NBT.GetInt(CurrLine);
					}

					CurrLine = a_NBT.FindChildByName(Pos, "Z");
					if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
					{
						Position.z = a_NBT.GetInt(CurrLine);
					}
				}
			}
			else
			{
				CurrLine = a_NBT.FindChildByName(Frame, "entity_id");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
				{
					// Post-1.20
					EntityId = a_NBT.GetInt(CurrLine);

					CurrLine = a_NBT.FindChildByName(Frame, "rotation");
					if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
					{
						Yaw = a_NBT.GetInt(CurrLine);
					}

					int Pos = a_NBT.FindChildByName(Frame, "pos");
					if ((Pos >= 0) && (a_NBT.GetType(Pos) == TAG_IntArray))
					{
						size_t PosLength = a_NBT.GetDataLength(Pos);
						if (PosLength == 12)
						{
							const auto * PosData = (a_NBT.GetData(Pos));
							Position.x = GetBEInt(PosData + 0);
							Position.y = GetBEInt(PosData + 4);
							Position.z = GetBEInt(PosData + 8);
						}
					}
				}
				else
				{
					// Well, I don't know what to make of this...
					continue;
				}
			}

			// Cuberite also has non-item frame decorators that may have been added
			// by plugins. We will have type and icon for those.
			CurrLine = a_NBT.FindChildByName(Frame, "Type");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
			{
				Type = static_cast<cMap::DecoratorType>(a_NBT.GetByte(CurrLine));
			}

			CurrLine = a_NBT.FindChildByName(Frame, "Icon");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
			{
				Icon = static_cast<cMap::icon>(a_NBT.GetByte(CurrLine));
			}

			m_Map->AddDecorator(Type, EntityId, Icon, Position, Yaw);
		}
	}

	CurrLine = a_NBT.FindChildByName(Data, "colors");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_ByteArray))
	{
		const Byte * data = reinterpret_cast<const Byte *>(a_NBT.GetData(CurrLine));
		for (unsigned int y = 0; y < m_Map->MAP_HEIGHT; y++)
		{
			for (unsigned int x = 0; x < m_Map->MAP_WIDTH; x++)
			{
				m_Map->SetPixel(x, y, data[y * m_Map->MAP_WIDTH + x]);
			}
		}
	}

	return true;
}





cIDCountSerializer::cIDCountSerializer(const AString & a_WorldName) : m_MapCount(0)
{
	auto DataPath = fmt::format(FMT_STRING("{}{}data"), a_WorldName, cFile::PathSeparator());
	m_Path = fmt::format(FMT_STRING("{}{}idcounts.dat"), DataPath, cFile::PathSeparator());
	cFile::CreateFolder(DataPath);
}





bool cIDCountSerializer::Load()
{
	AString Data = cFile::ReadWholeFile(m_Path);
	if (Data.empty())
	{
		return false;
	}

	// NOTE: idcounts.dat is not compressed (raw format)

	// Parse the NBT data:
	cParsedNBT NBT({ reinterpret_cast<const std::byte *>(Data.data()), Data.size() });
	if (!NBT.IsValid())
	{
		// NBT Parsing failed
		return false;
	}

	// Older formats didn't have a "data" container
	int DataSection = NBT.FindChildByName(0, "data");
	if (DataSection < 0)
	{
		DataSection = 0;
	}

	int CurrLine = NBT.FindChildByName(DataSection, "map");
	if (CurrLine >= 0)
	{
		// Older map counts were Shorts, newer are Ints
		switch (NBT.GetType(CurrLine))
		{
			case TAG_Int:
				m_MapCount = static_cast<unsigned int>(NBT.GetInt(CurrLine) + 1);
				break;

			case TAG_Short:
				m_MapCount = static_cast<unsigned int>(NBT.GetShort(CurrLine) + 1);
				break;

			default:
			{
				LOGERROR(fmt::format(FMT_STRING("Unexpected type {} for map count in {}"), NBT.GetType(CurrLine), m_Path));
				break;
			}
		}
	}
	else
	{
		LOGERROR(fmt::format(FMT_STRING("Don't understand the contents of {}"), m_Path));
		m_MapCount = 0;
		return false;
	}

	return true;
}





bool cIDCountSerializer::Save(void)
{
	cFastNBTWriter Writer;

	if (m_MapCount > 0)
	{
		Writer.AddShort("map", static_cast<Int16>(m_MapCount - 1));
	}

	Writer.Finish();

	#ifndef NDEBUG
	cParsedNBT TestParse(Writer.GetResult());
	ASSERT(TestParse.IsValid());
	#endif  // !NDEBUG

	cFile File;
	if (!File.Open(m_Path, cFile::fmWrite))
	{
		return false;
	}

	// NOTE: idcounts.dat is not compressed (raw format)

	File.Write(Writer.GetResult().data(), Writer.GetResult().size());
	File.Close();

	return true;
}
