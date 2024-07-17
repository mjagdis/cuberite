
// MapSerializer.cpp


#include "Globals.h"
#include "MapSerializer.h"
#include "OSSupport/GZipFile.h"
#include "FastNBT.h"

#include "json/json.h"
#include "../JsonUtils.h"
#include "../Map.h"
#include "../World.h"





namespace
{
	const char * ColourName[] =
	{
		"white",
		"orange",
		"magenta",
		"light_blue",
		"yellow",
		"lime",
		"pink",
		"gray",
		"light_gray",
		"cyan",
		"purple",
		"blue",
		"brown",
		"green",
		"red",
		"black"
	};




	const char * ColourToName(int a_Colour)
	{
		return ColourName[a_Colour & 0x0f];
	}





	unsigned int ColourFromName(const AString a_Name)
	{
		for (unsigned int i = 0; i < sizeof(ColourName) / sizeof(ColourName[0]); i++)
		{
			if (!a_Name.compare(ColourName[i]))
			{
				return i;
			}
		}

		return 0;
	}
}





cMapSerializer::cMapSerializer(const AString & a_WorldDataPath, cMap * a_Map):
	m_Map(a_Map)
{
	auto DataPath = GetDataPath(a_WorldDataPath);
	m_Path = fmt::format(FMT_STRING("{}{}map_{}.dat"), DataPath, cFile::PathSeparator(), a_Map->GetID());
	cFile::CreateFolder(DataPath);
}





bool cMapSerializer::Load()
{
	try
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
	catch (...)
	{
		return false;
	}
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
	cCSLock Lock(m_Map->m_CS);

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
		if ((itr.first.m_Type == cMap::DecoratorType::FRAME) || (itr.first.m_Type == cMap::DecoratorType::PERSISTENT))
		{
			a_Writer.BeginCompound("");

			a_Writer.AddInt("EntityId", static_cast<Int32>(itr.first.m_Id));
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

	a_Writer.BeginList("banners", TAG_Compound);
	for (const auto & itr : m_Map->GetDecorators())
	{
		if (itr.first.m_Type == cMap::DecoratorType::BANNER)
		{
			a_Writer.BeginCompound("");

			a_Writer.BeginCompound("Pos");
				a_Writer.AddInt("X", itr.second.m_Position.x);
				a_Writer.AddInt("Y", itr.second.m_Position.y);
				a_Writer.AddInt("Z", itr.second.m_Position.z);
			a_Writer.EndCompound();

			if (itr.second.m_Icon > 10)
			{
				a_Writer.AddString("Color", ColourToName(itr.second.m_Icon - 10));
			}

			if (itr.second.m_Name.size() > 0)
			{
				// Java Edition 1.13 saves this as a JSON object. 1.20 has it as
				// a quoted string.
				a_Writer.AddString("Name", JsonUtils::SerializeSingleValueJsonObject("text", itr.second.m_Name));
			}

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

	cCSLock Lock(m_Map->m_CS);

	int CurrLine = a_NBT.FindChildByName(Data, "scale");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		unsigned int Scale = static_cast<unsigned int>(a_NBT.GetByte(CurrLine));
		m_Map->SetScale(Scale);
	}

#if 0
	// Although dimension is stored in map files (because Java Edition) Cuberite is
	// multi-world, maps are associated with worlds and the dimension is not used.
	CurrLine = a_NBT.FindChildByName(Data, "dimension");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
	{
		// Careful! GetByte returns unsigned char but eDimension is an enum (signed int)
		// that contains both -1 and 255. We need to change the unsigned char to signed
		// before extending to eDimension.
		eDimension Dimension = static_cast<eDimension>(static_cast<signed char>(a_NBT.GetByte(CurrLine)));
	}
#endif

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
			cMap::eMapIcon Icon = cMap::eMapIcon::E_MAP_ICON_GREEN_ARROW;
			Vector3d Position;
			UInt32 EntityId = 0;
			int Yaw = 0;

			// The decoration structure changed somewhat arbitrarily (IMHO) in 1.20.
			// Cuberite currently saves using the pre-1.20 style.
			CurrLine = a_NBT.FindChildByName(Frame, "EntityId");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Int))
			{
				// Pre-1.20
				EntityId = static_cast<UInt32>(a_NBT.GetInt(CurrLine));

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
					EntityId = static_cast<UInt32>(a_NBT.GetInt(CurrLine));

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
			// by plugins. Cuberite saves them as frames with addition of type and icon.
			CurrLine = a_NBT.FindChildByName(Frame, "Type");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
			{
				Type = static_cast<cMap::DecoratorType>(a_NBT.GetByte(CurrLine));
			}

			CurrLine = a_NBT.FindChildByName(Frame, "Icon");
			if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_Byte))
			{
				Icon = static_cast<cMap::eMapIcon>(a_NBT.GetByte(CurrLine));
			}

			m_Map->AddDecorator(Type, EntityId, Icon, Position, Yaw, "");
		}
	}

	int Banners = a_NBT.FindChildByName(Data, "banners");
	if ((Banners >= 0) && (a_NBT.GetType(Banners) == TAG_List))
	{
		for (int Banner = a_NBT.GetFirstChild(Banners); Banner >= 0; Banner = a_NBT.GetNextSibling(Banner))
		{
			Vector3i Position;
			unsigned int Colour = 0;
			AString Name = "";

			// The decoration structure changed somewhat arbitrarily (IMHO) in Java Edition 1.20.
			// Cuberite currently saves using the pre-1.20 style.
			int Pos = a_NBT.FindChildByName(Banner, "Pos");
			if ((Pos >= 0) && (a_NBT.GetType(Pos) == TAG_Compound))
			{
				// Java Edition before 1.20
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

				CurrLine = a_NBT.FindChildByName(Banner, "Color");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_String))
				{
					Colour = ColourFromName(a_NBT.GetString(CurrLine));
				}

				CurrLine = a_NBT.FindChildByName(Banner, "Name");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_String))
				{
					Name = a_NBT.GetString(CurrLine);
				}
			}
			else if ((Pos >= 0) && (a_NBT.GetType(Pos) == TAG_IntArray))
			{
				// Java Edition 1.20 and later
				size_t DataLength = a_NBT.GetDataLength(Pos);
				if (DataLength == 12)
				{
					const auto * PosData = (a_NBT.GetData(Pos));
					Position.x = GetBEInt(PosData + 0);
					Position.y = GetBEInt(PosData + 4);
					Position.z = GetBEInt(PosData + 8);
				}

				CurrLine = a_NBT.FindChildByName(Banner, "color");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_String))
				{
					Colour = ColourFromName(a_NBT.GetString(CurrLine));
				}

				CurrLine = a_NBT.FindChildByName(Banner, "name");
				if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_String))
				{
					Name = a_NBT.GetString(CurrLine);
				}
			}
			else
			{
				// Well, I don't know what to make of this...
				continue;
			}

			// Java Edition 1.13 has name as a JSON object. 1.20 has it as a quoted string.
			if (Name[0] == '{')
			{
				Json::Value root;
				if (JsonUtils::ParseString(Name, root))
				{
					if (root.isObject())
					{
						const auto & txt = root["text"];
						if (txt.isString())
						{
							Name = txt.asString();
						}
					}
					else if (root.isString())
					{
						Name = root.asString();
					}
				}
			}

			m_Map->AddBanner(15 - Colour, Position, Name);
		}
	}

	CurrLine = a_NBT.FindChildByName(Data, "colors");
	if ((CurrLine >= 0) && (a_NBT.GetType(CurrLine) == TAG_ByteArray) && (a_NBT.GetDataLength(CurrLine) == m_Map->GetNumPixels()))
	{
		const Byte * data = reinterpret_cast<const Byte *>(a_NBT.GetData(CurrLine));
		for (UInt8 y = 0; y < m_Map->MAP_HEIGHT; y++)
		{
			for (UInt8 x = 0; x < m_Map->MAP_WIDTH; x++)
			{
				m_Map->SetPixel(x, y, data[y * m_Map->MAP_WIDTH + x]);
			}
		}
	}

	return true;
}





cIDCountSerializer::cIDCountSerializer(const AString & a_WorldDataPath) :
	m_MapCount(0)
{
	auto DataPath = cMapSerializer::GetDataPath(a_WorldDataPath);
	m_Path = fmt::format(FMT_STRING("{}{}idcounts.dat"), DataPath, cFile::PathSeparator());
	cFile::CreateFolder(DataPath);
}





bool cIDCountSerializer::Load()
{
	// FIXME: later versions of Java Edition compress idcounts.dat
	AString Data = cFile::ReadWholeFile(m_Path);
	if (Data.empty())
	{
		Data = cFile::ReadWholeFile(m_Path);
		if (Data.empty())
		{
			return false;
		}
	}

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
				m_MapCount = static_cast<UInt32>(NBT.GetInt(CurrLine)) + 1;
				break;

			case TAG_Short:
				m_MapCount = static_cast<UInt32>(NBT.GetShort(CurrLine)) + 1;
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
		return false;
	}

	return true;
}





bool cIDCountSerializer::Save(void)
{
	// NOTE: Cuberite never writes a compressed idcounts.dat and always uses
	// short for the map value.

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

	File.Write(Writer.GetResult().data(), Writer.GetResult().size());
	File.Close();

	return true;
}
