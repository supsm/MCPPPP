/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

constexpr auto VERSION = "0.7.0"; // MCPPPP version
constexpr int PACK_VER = 8; // pack.mcmeta pack format

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vmt
{
	// list of biomes
	inline const std::array<std::string, 98> biomelist =
	{
		"badlands",
		"badlands_plateau",
		"bamboo_jungle",
		"bamboo_jungle_hills",
		"basalt_deltas",
		"beach",
		"birch_forest",
		"birch_forest_hills",
		"cold_ocean",
		"crimson_forest",
		"dark_forest",
		"dark_forest_hills",
		"deep_cold_ocean",
		"deep_frozen_ocean",
		"deep_lukewarm_ocean",
		"deep_ocean",
		"deep_warm_ocean",
		"desert",
		"desert_hills",
		"desert_lakes",
		"dripstone_caves",
		"end_barrens",
		"end_highlands",
		"end_midlands",
		"eroded_badlands",
		"flower_forest",
		"forest",
		"frozen_ocean",
		"frozen_peaks",
		"frozen_river",
		"giant_spruce_taiga",
		"giant_spruce_taiga_hills",
		"giant_tree_taiga",
		"giant_tree_taiga_hills",
		"gravelly_mountains",
		"grove",
		"ice_spikes",
		"jagged_peaks",
		"jungle",
		"jungle_edge",
		"jungle_hills",
		"lukewarm_ocean",
		"lush_caves",
		"meadow",
		"modified_badlands_plateau",
		"modified_gravelly_mountains",
		"modified_jungle",
		"modified_jungle_edge",
		"modified_wooded_badlands_plateau",
		"mountain_edge",
		"mountains",
		"mushroom_field_shore",
		"mushroom_fields",
		"nether_wastes",
		"ocean",
		"old_growth_birch_forest",
		"old_growth_pine_taiga",
		"old_growth_spruce_taiga",
		"plains",
		"river",
		"savanna",
		"savanna_plateau",
		"shattered_savanna",
		"shattered_savanna_plateau",
		"small_end_islands",
		"snowy_beach",
		"snowy_mountains",
		"snowy_plains",
		"snowy_slopes",
		"snowy_taiga",
		"snowy_taiga_hills",
		"snowy_taiga_mountains",
		"snowy_tundra",
		"soul_sand_valley",
		"sparse_jungle",
		"stone_shore",
		"stony_peaks",
		"stony_shore",
		"sunflower_plains",
		"swamp",
		"swamp_hills",
		"taiga",
		"taiga_hills",
		"taiga_mountains",
		"tall_birch_forest",
		"tall_birch_hills",
		"the_end",
		"the_void",
		"warm_ocean",
		"warped_forest",
		"windswept_forest",
		"windswept_gravelly_hills",
		"windswept_hills",
		"windswept_savanna",
		"wooded_badlands",
		"wooded_badlands_plateau",
		"wooded_hills",
		"wooded_mountains"
	};


	// list of healths for each valid mob
	inline const std::unordered_map<std::string, int> mob_healths =
	{
		{ "axolotl", 14 },
		{ "bat", 6 },
		{ "bee", 10 },
		{ "blaze", 20 },
		{ "cat", 10 },
		{ "cave_spider", 12 },
		{ "chicken", 4 },
		{ "cod", 3 },
		{ "cow", 10 },
		{ "creeper", 20 },
		{ "dolphin", 10 },
		{ "donkey", 30 },
		{ "drowned", 20 },
		{ "elder_guardian", 80 },
		{ "ender_dragon", 200 },
		{ "enderman", 40 },
		{ "endermite", 8 },
		{ "evoker", 24 },
		{ "fox", 10 },
		{ "ghast", 10 },
		{ "giant", 100 },
		{ "glow_squid", 10 },
		{ "goat", 10 },
		{ "guardian", 30 },
		{ "hoglin", 40 },
		{ "horse", 30 },
		{ "husk", 20 },
		{ "illusioner", 32 },
		{ "iron_golem", 100 },
		{ "llama", 30 },
		{ "magma_cube", 16 },
		{ "mooshroom", 10 },
		{ "mule", 30 },
		{ "ocelot", 10 },
		{ "panda", 20 },
		{ "parrot", 6 },
		{ "phantom", 20 },
		{ "pig", 10 },
		{ "piglin", 16 },
		{ "piglin_brute", 50 },
		{ "pillager", 24 },
		{ "polar_bear", 30 },
		{ "pufferfish", 3 },
		{ "rabbit", 3 },
		{ "ravager", 100 },
		{ "salmon", 3 },
		{ "sheep", 8 },
		{ "shulker", 30 },
		{ "silverfish", 8 },
		{ "skeleton", 20 },
		{ "skeleton_horse", 15 },
		{ "slime", 16 },
		{ "snow_golem", 4 },
		{ "spider", 16 },
		{ "squid", 10 },
		{ "stray", 20 },
		{ "strider", 20 },
		{ "trader_llama", 30 },
		{ "tropical_fish", 3 },
		{ "turtle", 30 },
		{ "vex", 14 },
		{ "villager", 20 },
		{ "vindicator", 24 },
		{ "wandering_trader", 20 },
		{ "witch", 26 },
		{ "wither", 300 },
		{ "wither_skeleton", 20 },
		{ "wolf", 20 },
		{ "zoglin", 40 },
		{ "zombie", 20 },
		{ "zombie_horse", 15 },
		{ "zombie_villager", 20 },
		{ "zombified_piglin", 20 },
	};


	// list of mobs reselect supports, excluding "special" mobs
	inline const std::array<std::string, 63> normal_mobs =
	{
		"bat",
		"bee",
		"blaze",
		"cave_spider",
		"chicken",
		"cod",
		"cow",
		"creeper",
		"dolphin",
		"donkey",
		"drowned",
		"elder_guardian",
		"ender_dragon",
		"enderman",
		"endermite",
		"evoker",
		"ghast",
		"giant",
		"glow_squid",
		"goat",
		"guardian",
		"hoglin",
		"husk",
		"illusioner",
		"iron_golem",
		"llama",
		"magma_cube",
		"mule",
		"ocelot",
		"phantom",
		"pig",
		"piglin",
		"piglin_brute",
		"pillager",
		"polar_bear",
		"pufferfish",
		"ravager",
		"salmon",
		"sheep",
		"silverfish",
		"skeleton",
		"skeleton_horse",
		"slime",
		"snow_golem",
		"spider",
		"strider",
		"squid",
		"stray",
		"trader_llama",
		"turtle",
		"vex",
		"villager",
		"vindicator",
		"wandering_trader",
		"witch",
		"wither",
		"wither_skeleton",
		"wolf",
		"zoglin",
		"zombie",
		"zombie_horse",
		"zombie_villager",
		"zombified_piglin",
	};

	class special_mob
	{
	public:

		enum class match_type { type_only, type_name, name_type };

		std::string name, foldername;
		std::string reselect_func; // function to get variant/type in reselect, e.g. axolotl_variant, cat_type
		match_type match;
		std::vector<std::string> types; // valid types
		std::vector<std::string> typenames; // optional, type names te use instead of types

		// @param type  type (e.g. from split)
		// @return typename to use
		std::string get_typename(const std::string& type) const
		{
			if (typenames.empty())
			{
				return type;
			}
			const auto it = std::lower_bound(types.begin(), types.end(), type);
			const size_t ind = it - types.begin();
			return typenames.at(ind);
		}

		// @param source  name of texture (without extension)
		// @return pair of name and type
		std::pair<std::string, std::string> split(const std::string& source) const
		{
			if (match == match_type::type_only)
			{
				return { name, source };
			}
			std::string item, type;
			std::vector<std::string> items;
			std::stringstream ss(source);
			while (std::getline(ss, item, '_'))
			{
				if (!item.empty())
				{
					items.push_back(item);
				}
			}

			// set item to name
			item.clear();
			if (match == match_type::type_name)
			{
				item = items.back();
				items.pop_back();
			}
			else if (match == match_type::name_type)
			{
				item = items.front();
				items.erase(items.begin());
			}
			if (!items.empty())
			{
				for (const auto& i : items)
				{
					type += std::move(i) + '_';
				}
				type.pop_back(); // erase excess _ at end
			}

			return { item, type };
		}

		// @param source  pair of texture name and source folder
		bool operator==(const std::pair<std::string, std::string>& source) const
		{
			if (foldername != source.second)
			{
				return false;
			}
			switch (match)
			{
			case match_type::type_only:
				return std::binary_search(types.begin(), types.end(), source.first);
			}
			const auto p = split(source.first);
			return (p.first == name && std::binary_search(types.begin(), types.end(), p.second));
		}
	};

	// special mobs which have different types
	inline const std::array<special_mob, 11> special_mobs =
	{ {
		{
			"axolotl", "axolotl", "axolotl_variant", special_mob::match_type::name_type,
			{ "blue", "cyan", "gold", "lucy", "wild" }
		},
		{
			"cat", "cat", "cat_type", special_mob::match_type::type_only,
			{ "all_black", "black", "british_shorthair", "calico", "jellie", "ocelot", "persian", "ragdoll", "red", "siamese", "tabby", "white" }
		},
		{
			"fox", "fox", "fox_type", special_mob::match_type::name_type,
			{ "", "snow" }
		},
		{
			"horse", "horse", "horse_color", special_mob::match_type::name_type,
			{ "black", "brown", "chestnut", "creamy", "darkbrown", "gray", "white" }
		},
		{
			"horse", "horse", "horse_marking", special_mob::match_type::name_type,
			{ "markings_blackdots", "markings_white", "markings_whitedots", "markings_whitefield" }
		},
		{
			"mooshroom", "cow", "mooshroom_type", special_mob::match_type::type_name,
			{ "brown", "red" }
		},
		{
			"panda", "panda", "panda_gene", special_mob::match_type::type_name,
			{ "", "aggressive", "brown", "lazy", "playful", "weak", "worried" }
		},
		{
			"parrot", "parrot", "parrot_variant", special_mob::match_type::name_type,
			{ "blue", "green", "grey", "red_blue", "yellow_blue" }
		},
		/*{
			"rabbit", "rabbit", "rabbit_variant", special_mob::match_type::type_only,
			{ "black", "brown", "caerbannog", "gold", "salt", "toast", "white", "white_splotched" }
		},*/// 2/12/22: "rabbit_variant is broken"
		{
			"shulker", "shulker", "shulker_color", special_mob::match_type::name_type,
			{ "", "black", "blue", "brown", "cyan", "gray", "green", "light_blue", "light_gray", "lime", "magenta", "orange", "pink", "purple", "red", "white", "yellow" }
		},
		{
			"tropical_fish", "fish", "tropical_fish_variant", special_mob::match_type::name_type,
			{ "a_pattern_1", "a_pattern_2", "a_pattern_3", "a_pattern_4", "a_pattern_5", "a_pattern_6", "b_pattern_1", "b_pattern_2", "b_pattern_3", "b_pattern_4", "b_pattern_5", "b_pattern_6" },
			{ "kob",         "sunstreak",   "snooper",     "dasher",      "brinely",     "spotty",      "flopper",     "stripey",     "glitter",     "blockfish",   "betty",       "clayfish" }
		}
	} };
}
