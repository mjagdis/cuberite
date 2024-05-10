return
{
	HOOK_CONSUME_INGREDIENTS =
	{
		CalledWhen = "Called after a player has clicked to collect a crafted item but before the item is received or ingredients consumed.",
		DefaultFnName = "OnConsumeIngredients",  -- also used as pagename
		Desc = [[
			This hook is called after a {{cPlayer|player}} has clicked to collect a crafted item but before
			the item is actually received or ingredients consumed.
			Plugins may use this hook to modify the ingredients and/or result after the player has committed
			to crafting. For instance to add metadata to the result that should only be created if the
			item is actually being created. Or just to randomize the result or ingredients!</p>
		]],
		Params =
		{
			{ Name = "Player", Type = "{{cPlayer}}", Notes = "The player who has committed to crafting" },
			{ Name = "Grid", Type = "{{cCraftingGrid}}", Notes = "The crafting grid contents" },
			{ Name = "Recipe", Type = "{{cCraftingRecipe}}", Notes = "The recipe that Cuberite will use. Modify this object to change the recipe" },
		},
		Returns = [[
			If the function returns false or no value, other plugins' callbacks are called and then Cuberite
			completes the crafting and gives the result to the player.</p>
			<p>
			If the function returns true, no other callbacks are called for this event and Cuberite cancels
			the crafting. In this case, however, the client will be temporarily and unavoidably confused.
			It will appear that the player has received the item (without the ingredients being consumed)
			but the new item will eventually disappear.
		]],
	},  -- HOOK_PRE_CRAFTING
}





