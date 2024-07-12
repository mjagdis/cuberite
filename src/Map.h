
// Map.h

// Implementation of in-game coloured maps





#pragma once

#include <shared_mutex>

#include "Defines.h"
#include "ChunkDef.h"
#include "Chunk.h"
#include "FastRandom.h"




/**
Java Edition: maps for a different dimension to the player
retain the player's last position in the map's dimension
and the marker is always static (not spinning).
FIXME: true for 1.12.2, check for latest

Bedrock Edition: the marker changes colour depending on the
player's dimension and the player's position is transformed
to the map's dimension's coordinates. If no transform exists
(such as a nether map in the end or an end map in the overworld
or nether) the marker is not displayed.

Cuberite: markers are only shown if the player is in the
same world as the map.
*/





class cClientHandle;
class cWorld;
class cPlayer;
class cMap;





// tolua_begin

/** Encapsulates an in-game world map. */
class cMap :
	public std::enable_shared_from_this<cMap>
{
public:
	enum eMapIcon
	{
		E_MAP_ICON_NONE               = -1,  ///< Used for plain non-locator maps.
		E_MAP_ICON_WHITE_ARROW        = 0,  ///< Used for players.
		E_MAP_ICON_GREEN_ARROW        = 1,  ///< Used for item frames.
		E_MAP_ICON_RED_ARROW          = 2,
		E_MAP_ICON_BLUE_ARROW         = 3,
		E_MAP_ICON_WHITE_CROSS        = 4,
		E_MAP_ICON_RED_TRIANGLE       = 5,
		E_MAP_ICON_WHITE_CIRCLE       = 6,  ///< Player outside the boundaries of the map.
		E_MAP_ICON_SMALL_WHITE_CIRCLE = 7,  ///< Player far outside the boundaries of the map.
		E_MAP_ICON_MANSION            = 8,
		E_MAP_ICON_MONUMENT           = 9,

		// Banners and above are Java Edition 1.13+, not present in Bedrock
		E_MAP_ICON_WHITE_BANNER       = 10,
		E_MAP_ICON_ORANGE_BANNER      = 11,
		E_MAP_ICON_MAGENTA_BANNER     = 12,
		E_MAP_ICON_LIGHT_BLUE_BANNER  = 13,
		E_MAP_ICON_YELLOW_BANNER      = 14,
		E_MAP_ICON_LIME_BANNER        = 15,
		E_MAP_ICON_PINK_BANNER        = 16,
		E_MAP_ICON_GRAY_BANNER        = 17,
		E_MAP_ICON_LIGHT_GRAY_BANNER  = 18,
		E_MAP_ICON_CYAN_BANNER        = 19,
		E_MAP_ICON_PURPLE_BANNER      = 20,
		E_MAP_ICON_BLUE_BANNER        = 21,
		E_MAP_ICON_BROWN_BANNER       = 22,
		E_MAP_ICON_GREEN_BANNER       = 23,
		E_MAP_ICON_RED_BANNER         = 24,
		E_MAP_ICON_BLACK_BANNER       = 25,

		E_MAP_ICON_TREASURE_MARKER    = 26,  // aka red cross

		// Other markers in Java / Bedrock Edition versions beyond what we support
		// E_MAP_ICON_DESERT_VILLAGE  = 27,
		// E_MAP_ICON_PLAINS_VILLAGE  = 28,
		// E_MAP_ICON_SAVANNA_VILLAGE = 29,
		// E_MAP_ICON_SNOWY_VILLAGE   = 30,
		// E_MAP_ICON_TAIGA_VILLAGE   = 31,
		// E_MAP_ICON_JUNGLE_PYRAMID  = 32,
		// E_MAP_ICON_SWAMP_HUT       = 33,
		// E_MAP_ICON_MAGENTA_ARROW   = xx,  // Bedrock only, unused
		// E_MAP_ICON_ORANGE_ARROW    = xx,  // Bedrock only, unused
		// E_MAP_ICON_YELLOW_ARROW    = xx,  // Bedrock only, unused
		// E_MAP_ICON_CYAN_ARROW      = xx,  // Bedrock only, unused
		// E_MAP_ICON_GREEN_TRIANGLE  = xx,  // Bedrock only, stronghold, fortress, etc. on an explorer map
	};

	// tolua_end

	enum class DecoratorType
	{
		PLAYER = 0,
		FRAME,
		BANNER,
		PERSISTENT,
	};

	/** Encapsulates a map decorator.
	A map decorator represents an object drawn on the map that can move freely.
	(e.g. player trackers and item frame pointers)

	Excluding manually placed decorators,
	decorators are automatically managed (allocated and freed) by their parent cMap instance.
	*/
	class Decorator
	{
	public:
		class Key
		{
		public:
			Key(cMap::DecoratorType a_Type, UInt32 a_Id) :
				m_Type(a_Type),
				m_Id(a_Id)
			{
			}

			bool operator < (const cMap::Decorator::Key & a_Rhs) const
			{
				return (m_Type < a_Rhs.m_Type)
				|| ((m_Type == a_Rhs.m_Type) && (m_Id < a_Rhs.m_Id));
			}

			const cMap::DecoratorType m_Type;
			const UInt32 m_Id;
		};

		class Value
		{
		private:
			static UInt8 YawToRot(double a_Yaw)
			{
				return CeilC(((a_Yaw - 11.25) * 16) / 360) & 0x0f;
			}

		public:

			Value(eMapIcon a_Icon, const Vector3i & a_Position, double a_Yaw, Int8 a_MapX, Int8 a_MapZ, const AString & a_Name) :
				m_Icon(a_Icon),
				m_Position(a_Position),
				m_Yaw(FloorC(a_Yaw)),
				m_MapX(a_MapX),
				m_MapZ(a_MapZ),
				m_CurrentRot(YawToRot(a_Yaw)),
				m_Spin(0),
				m_SpinTime(0),
				m_SpinRate(0),
				m_Name(a_Name)
			{
			}

			bool Update(eMapIcon a_Icon, const Vector3i & a_Position, double a_Yaw, Int8 a_MapX, Int8 a_MapZ, bool a_Spinning, const AString & a_Name)
			{
				UInt8 Rot = m_CurrentRot;

				if (!a_Spinning)
				{
					Rot = YawToRot(a_Yaw);
				}

				if ((a_MapX != m_MapX) || (a_MapZ != m_MapZ) || (Rot != m_CurrentRot) || (a_Icon != m_Icon) || m_Name.compare(a_Name))
				{
					m_Icon = a_Icon;
					m_Position = a_Position;
					m_Yaw = FloorC(a_Yaw);
					m_MapX = a_MapX;
					m_MapZ = a_MapZ;
					m_CurrentRot = Rot;
					m_Name = a_Name;
					return true;
				}

				return false;
			}

			bool Spin(void)
			{
				if (m_Icon <= E_MAP_ICON_BLUE_ARROW)
				{
					if ((m_SpinTime == 0) || (--m_SpinTime == 0))
					{
						m_SpinRate = 1 + GetRandomProvider().RandInt<Int8>(3);
						m_SpinTime = 8 + GetRandomProvider().RandInt<UInt8>(12) / static_cast<UInt8>(m_SpinRate);
						m_SpinRate = (GetRandomProvider().RandInt(1) > 0 ? m_SpinRate : -m_SpinRate);
					}

					m_Spin += m_SpinRate;

					UInt8 Rot = (m_Spin / 2) & 0x0f;
					bool ret = (Rot != m_CurrentRot);
					m_CurrentRot = Rot;

					return ret;
				}

				return false;
			}

			eMapIcon m_Icon;

			Vector3i m_Position;
			int m_Yaw;

			Int8 m_MapX;
			Int8 m_MapZ;

			UInt8 m_CurrentRot;

			UInt8 m_Spin, m_SpinTime;
			Int8 m_SpinRate;

			AString m_Name;
		};
	};

	// tolua_begin

	enum eMapColor
	{
		E_MAP_COLOR_AIR = 0,
		E_MAP_COLOR_TRANSPARENT = E_MAP_COLOR_AIR,
		E_MAP_COLOR_GRASS,
		E_MAP_COLOR_SAND,
		E_MAP_COLOR_WOOL,
		E_MAP_COLOR_FIRE,
		E_MAP_COLOR_ICE,
		E_MAP_COLOR_METAL,
		E_MAP_COLOR_PLANT,
		E_MAP_COLOR_SNOW,
		E_MAP_COLOR_WHITE = E_MAP_COLOR_SNOW,
		E_MAP_COLOR_CLAY,
		E_MAP_COLOR_DIRT,
		E_MAP_COLOR_STONE,
		E_MAP_COLOR_WATER,
		E_MAP_COLOR_WOOD,
		E_MAP_COLOR_QUARTZ,
		E_MAP_COLOR_ORANGE,
		E_MAP_COLOR_MAGENTA,
		E_MAP_COLOR_LIGHTBLUE,
		E_MAP_COLOR_YELLOW,
		E_MAP_COLOR_LIME,
		E_MAP_COLOR_LIGHTGREEN = E_MAP_COLOR_LIME,
		E_MAP_COLOR_PINK,
		E_MAP_COLOR_GRAY,
		E_MAP_COLOR_LIGHTGRAY,
		E_MAP_COLOR_CYAN,
		E_MAP_COLOR_PURPLE,
		E_MAP_COLOR_BLUE,
		E_MAP_COLOR_BROWN,
		E_MAP_COLOR_OLIVE,
		E_MAP_COLOR_GREEN = E_MAP_COLOR_OLIVE,
		E_MAP_COLOR_TERRACOTTA,
		E_MAP_COLOR_RED = E_MAP_COLOR_TERRACOTTA,
		E_MAP_COLOR_BLACK,
		E_MAP_COLOR_GOLD,
		E_MAP_COLOR_DIAMOND,
		E_MAP_COLOR_SEA_BLUE = E_MAP_COLOR_DIAMOND,
		E_MAP_COLOR_LAPIS_LAZULI,
		E_MAP_COLOR_EMERALD,
		E_MAP_COLOR_PODZOL,
		E_MAP_COLOR_NETHER,
		E_MAP_COLOR_BRICK_RED = E_MAP_COLOR_NETHER,
		E_MAP_COLOR_WHITE_TERRACOTA,
		E_MAP_COLOR_ORANGE_TERRACOTA,
		E_MAP_COLOR_MAGENTA_TERRACOTA,
		E_MAP_COLOR_LIGHT_BLUE_TERRACOTA,
		E_MAP_COLOR_YELLOW_TERRACOTA,
		E_MAP_COLOR_LIME_TERRACOTA,
		E_MAP_COLOR_PINK_TERRACOTA,
		E_MAP_COLOR_GRAY_TERRACOTA,
		E_MAP_COLOR_LIGHT_GRAY_TERRACOTA,
		E_MAP_COLOR_CYAN_TERRACOTA,
		E_MAP_COLOR_PURPLE_TERRACOTA,
		E_MAP_COLOR_BLUE_TERRACOTA,
		E_MAP_COLOR_BROWN_TERRACOTA,
		E_MAP_COLOR_GREEN_TERRACOTA,
		E_MAP_COLOR_RED_TERRACOTA,
		E_MAP_COLOR_BLACK_TERRACOTA,
		E_MAP_COLOR_CRIMSON_NYLIUM,
		E_MAP_COLOR_CRIMSON_STEM,
		E_MAP_COLOR_CRIMSON_HYPHAE,
		E_MAP_COLOR_WARPED_NYLIUM,
		E_MAP_COLOR_WARPED_STEM,
		E_MAP_COLOR_WARPED_HYPHAE,
		E_MAP_COLOR_WARPED_WART_BLOCK,
		E_MAP_COLOR_DEEPSLATE,
		E_MAP_COLOR_RAW_IRON,
		E_MAP_COLOR_GLOW_LICHEN,

		/** The following are for convenience and consistency since they are
		frequently used. */
		E_MAP_COLOR_BIRCH = E_MAP_COLOR_SAND,
		E_MAP_COLOR_JUNGLE = E_MAP_COLOR_DIRT,
		E_MAP_COLOR_OAK = E_MAP_COLOR_WOOD,
		E_MAP_COLOR_ACACIA = E_MAP_COLOR_ORANGE,
		E_MAP_COLOR_DARK_OAK = E_MAP_COLOR_BROWN,
		E_MAP_COLOR_SPRUCE = E_MAP_COLOR_PODZOL,
	};

	typedef Byte ColorID;

	// tolua_end

private:

	static const int MAP_WIDTH = 128;
	static const int MAP_HEIGHT = 128;
	static const int DEFAULT_RADIUS = 128;
	static const int DEFAULT_WATER_DEPTH = 16;
	static const int DEFAULT_TRACKING_DISTANCE = 320;
	static const int DEFAULT_FAR_TRACKING_DISTANCE = 1026;

	static const int SKETCH_WATER_COLOUR = (eMapColor::E_MAP_COLOR_ORANGE * 4);  // Colour for water. (No brightness.)
	static const int SKETCH_OUTLINE_COLOUR = (eMapColor::E_MAP_COLOR_BROWN * 4) | 3;  // Colour to outline water with.
	static const int SKETCH_CORNER_COLOUR = (eMapColor::E_MAP_COLOR_SAND * 4) | 0;  // Colour for outline corners to give them a slight anti-aliased look.

public:

	typedef std::set<cClientHandle *> cMapClientList;
	typedef std::multimap<Decorator::Key, Decorator::Value> cMapDecoratorList;

	/** Construct an empty map. */
	cMap(unsigned int a_ID, cWorld * a_World);

	/** Construct an empty map at the specified coordinates. */
	cMap(unsigned int a_ID, eMapIcon a_MapType, int a_X, int a_Z, cWorld * a_World, unsigned int a_Scale = 3);

	/** Construct a filled map as a copy of the given map. */
	cMap(unsigned int a_ID, cMap & a_Map);

	cMap(cMap & a_Map) = delete;
	cMap(const cMap & a_Map) = delete;

	/** Sends a map update to all registered clients
	Clears the list holding registered clients and decorators */
	void Tick();

	/** Update the decorator for this client, fill in the local area if the map
	is equipped and add the client to the list that will receive the next update. */
	void UpdateClient(const cPlayer * a_Player, bool a_IsEquipped);

	// tolua_begin

	// These should ONLY be called from inside a DoWithMap!

	void SetPosition(int a_CenterX, int a_CenterZ);

	void SetScale(unsigned int a_Scale) { m_Scale = a_Scale; }

	void SetLocked(bool a_OnOff);

	void SetTrackingPosition(bool a_OnOff);

	void SetUnlimitedTracking(bool a_OnOff);

	void SetTrackingThreshold(unsigned int a_Threshold);

	void SetFarTrackingThreshold(unsigned int a_Threshold);

	bool SetPixel(int a_X, int a_Z, ColorID a_Data);

	ColorID GetPixel(int a_X, int a_Z) const
	{
		return ((a_X >= 0) && (a_X < MAP_WIDTH) && (a_Z >= 0) && (a_Z < MAP_HEIGHT)) ? m_Data[a_Z * MAP_WIDTH + a_X] : 0;
	}

	int GetWidth (void) const { return MAP_WIDTH;  }
	int GetHeight(void) const { return MAP_HEIGHT; }

	unsigned int GetScale(void) const { return m_Scale; }

	int GetCenterX(void) const { return m_CenterX; }
	int GetCenterZ(void) const { return m_CenterZ; }

	unsigned int GetID(void) const { return m_ID; }

	cWorld * GetWorld(void) const { return m_World; }

	AString GetName(void) const { return m_Name; }

	eDimension GetDimension(void) const;

	unsigned int GetNumPixels(void) const { return MAP_WIDTH * MAP_HEIGHT; }

	int GetPixelWidth(void) const { return 1 << m_Scale; }

	bool GetLocked(void) const { return m_Locked; }

	bool GetTrackingPosition(void) const { return m_TrackingPosition; }

	bool GetUnlimitedTracking(void) const { return m_UnlimitedTracking; }

	unsigned int GetTrackingThreshold(void) const { return m_TrackingThreshold; }

	unsigned int GetFarTrackingThreshold(void) const { return m_FarTrackingThreshold; }

	unsigned int GetTrackingDistance(const Vector3i & a_Position) const
	{
		return std::max(abs(a_Position.x - m_CenterX), abs(a_Position.z - m_CenterZ));
	}

	void AddMarker(UInt32 a_Id, eMapIcon a_Icon, const Vector3i & a_Position, double a_Yaw, const AString & a_Name)
	{
		AddDecorator(DecoratorType::PERSISTENT, a_Id, a_Icon, a_Position, a_Yaw, a_Name);
	}

	void RemoveMarker(UInt32 a_Id)
	{
		RemoveDecorator(DecoratorType::PERSISTENT, a_Id);
	}

	// tolua_end

	void AddBanner(unsigned int a_Colour, const Vector3i & a_Position, const AString & a_Name)
	{
		UInt32 Id = DecoratorIdFromPosition(a_Position);

		AddDecorator(DecoratorType::BANNER, Id, BannerColourToIcon(a_Colour), a_Position, 180, a_Name);
	}

	void AddFrame(UInt32 a_Id, const Vector3i & a_Position, double a_Yaw)
	{
		AddDecorator(DecoratorType::FRAME, a_Id, E_MAP_ICON_GREEN_ARROW, a_Position, a_Yaw, "");
	}

	void RemoveFrame(UInt32 a_Id)
	{
		RemoveDecorator(DecoratorType::FRAME, a_Id);
	}

	void RemoveFrame(const Vector3i & a_Position, double a_Yaw);

	const cMapDecoratorList GetDecorators(void) const { return m_Decorators; }

	void AddClient(cClientHandle * a_Client) { m_ClientsInCurrentTick.emplace(a_Client); }

private:

	static constexpr int SnapToGrid(int a_Coord, unsigned int a_Scale)
	{
		return FAST_FLOOR_DIV(a_Coord, (1 << a_Scale) * cChunkDef::Width * 8)
			* (1 << a_Scale) * cChunkDef::Width * 8
			+ (1 << a_Scale) * cChunkDef::Width * 4;
	}

	static constexpr eMapIcon BannerColourToIcon(unsigned int a_Colour)
	{
		return static_cast<eMapIcon>(E_MAP_ICON_WHITE_BANNER + 15 - a_Colour);
	}

	UInt32 DecoratorIdFromPosition(const Vector3i & a_Position) const
	{
		int X = MAP_WIDTH / 2 + (a_Position.x - m_CenterX) / GetPixelWidth();
		int Z = MAP_HEIGHT / 2 + (a_Position.z - m_CenterZ) / GetPixelWidth();
		return static_cast<UInt32>((a_Position.y * 65536) | (X * 256) | Z);
	}

	bool WaterAt(int a_X, int a_Z);

	void Sketch(void);

	void FillAndSketch(void);

	bool InRange(int a_CenterX, int a_CenterZ, int a_TargetX, int a_TargetZ) const;

	/** Update a circular region around the specified player. */
	void UpdateRadius(const cPlayer * a_Player);

	/** Update the specified pixel. */
	ColourID PixelColour(cChunk & a_Chunk, int a_RelX, int a_RelZ) const;

	void AddDecorator(DecoratorType a_Type, UInt32 a_Id, eMapIcon a_Icon, const Vector3i & a_Position, double a_Yaw, const AString a_Name);

	void RemoveDecorator(DecoratorType a_Type, UInt32 a_Id);

	mutable cCriticalSection m_CS;

	unsigned int m_ID;

	/** The zoom level, 2^scale square blocks per pixel */
	unsigned int m_Scale;

	int m_CenterX;
	int m_CenterZ;

	/** The radius of the area around players that is updated when the map is held. */
	const int m_Radius;

	/** Indicates the map is in the process of being sketched. */
	int m_Busy;

	/** Indicates that the map has changes that need to be saved. */
	bool m_Dirty;

	/** Indicates that the map has changes that need to be sent to clients. */
	bool m_Send;

	UInt8 m_ChangeStartX, m_ChangeEndX, m_ChangeStartZ, m_ChangeEndZ;

	bool m_Locked;
	bool m_TrackingPosition;
	bool m_UnlimitedTracking;
	unsigned int m_TrackingThreshold;
	unsigned int m_FarTrackingThreshold;

	cWorld * m_World;

	cMapClientList m_ClientsInCurrentTick;
	cMapClientList m_ClientsWithCurrentData;

	cMapDecoratorList m_Decorators;

	AString m_Name;

	/** Column-major array of colours */
	ColorID m_Data[MAP_WIDTH * MAP_HEIGHT];

	friend class cMapManager;
	friend class cMapSerializer;
	friend class cMapUpdaterEngine;
	friend class cMapUpdater;
	friend class cMapSketchUpdater;

};  // tolua_export



