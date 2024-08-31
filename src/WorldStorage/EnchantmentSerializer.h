
#pragma once

class cEnchantments;
class cFastNBTWriter;
class cParsedNBT;





namespace EnchantmentSerializer
{

	/** Writes the enchantments into the specified NBT writer; begins with the LIST tag of the specified name ("ench" or "StoredEnchantments") uses numeric enchantment ids*/
	void WriteToNBTCompound(const cEnchantments & a_Enchantments, cFastNBTWriter & a_Writer, const AString & a_ListTagName);

	/** Writes the enchantments into the specified NBT writer; begins with the LIST tag of the specified name ("ench" or "StoredEnchantments") uses string enchantment ids */
	void WriteToNBTCompoundStrings(const cEnchantments & a_Enchantments, cFastNBTWriter & a_Writer, const AString & a_ListTagName);

	/** Reads the enchantments from the specified NBT list tag (ench or StoredEnchantments) */
	void ParseFromNBT(cEnchantments & a_Enchantments, const cParsedNBT & a_NBT, int a_EnchListTagIdx);

};




