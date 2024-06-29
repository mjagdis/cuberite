
#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockGlassHandler final :
	public cBlockHandler
{
public:

	using cBlockHandler::cBlockHandler;

private:

	virtual cItems ConvertToPickups(const NIBBLETYPE a_BlockMeta, const cItem * const a_Tool) const override
	{
		// Only drop self when mined with silk-touch:
		if (ToolHasSilkTouch(a_Tool))
		{
			return cItem(m_BlockType, 1, a_BlockMeta);
		}
		return {};
	}
} ;




