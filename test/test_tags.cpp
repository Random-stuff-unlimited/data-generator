#include "../out/minecraftTags.hpp"

#include <iostream>

int main() {
	std::cout << "=== Minecraft Tags Test Program ===\n\n";

	// Display basic statistics
	std::cout << "Total categories: " << getTagCategoryCount() << "\n";
	std::cout << "Total tags: " << getTotalTagCount() << "\n\n";

	// List all categories
	std::cout << "Available categories:\n";
	auto categoryNames = getTagCategoryNames();
	for (const auto& name : categoryNames) {
		std::cout << "  - " << name << "\n";
	}
	std::cout << "\n";

	// Show tags in each category with counts
	for (const auto& categoryName : categoryNames) {
		auto tagNames = getTagNames(categoryName);
		std::cout << "Category '" << categoryName << "' has " << tagNames.size() << " tags:\n";

		// Show first few tags as examples
		int count = 0;
		for (const auto& tagName : tagNames) {
			const auto* tag = getTag(categoryName, tagName);
			if (tag) {
				std::cout << "  - " << tagName << " (" << tag->values.size() << " values)\n";
				if (++count >= 5) {
					if (tagNames.size() > 5) {
						std::cout << "  ... and " << (tagNames.size() - 5) << " more\n";
					}
					break;
				}
			}
		}
		std::cout << "\n";
	}

	// Test specific tag lookups
	std::cout << "=== Testing specific tag lookups ===\n";

	// Test damage_type tags
	if (getTagCategory("damage_type")) {
		std::cout << "\nDamage type 'is_fire' tag contains:\n";
		const auto* fireTag = getTag("damage_type", "is_fire");
		if (fireTag) {
			for (const auto& value : fireTag->values) {
				std::cout << "  - " << value << "\n";
			}
		}

		// Test membership check
		bool isFireDamage = isInTag("damage_type", "is_fire", "minecraft:lava");
		std::cout << "\nIs 'minecraft:lava' fire damage? " << (isFireDamage ? "Yes" : "No") << "\n";

		bool isNotFireDamage = isInTag("damage_type", "is_fire", "minecraft:fall");
		std::cout << "Is 'minecraft:fall' fire damage? " << (isNotFireDamage ? "Yes" : "No") << "\n";
	}

	// Test worldgen biome tags
	if (getTagCategory("worldgen")) {
		std::cout << "\nWorldgen 'biome/is_ocean' tag contains:\n";
		const auto* oceanTag = getTag("worldgen", "biome/is_ocean");
		if (oceanTag) {
			for (const auto& value : oceanTag->values) {
				std::cout << "  - " << value << "\n";
			}
		}

		// Test membership check
		bool isOcean = isInTag("worldgen", "biome/is_ocean", "minecraft:ocean");
		std::cout << "\nIs 'minecraft:ocean' an ocean biome? " << (isOcean ? "Yes" : "No") << "\n";
	}

	// Test enchantment tags
	if (getTagCategory("enchantment")) {
		std::cout << "\nSome enchantment tags:\n";
		auto enchantmentTags = getTagNames("enchantment");
		int	 count			 = 0;
		for (const auto& tagName : enchantmentTags) {
			const auto* tag = getTag("enchantment", tagName);
			if (tag && !tag->values.empty()) {
				std::cout << "  - " << tagName << ": " << tag->values[0];
				if (tag->values.size() > 1) {
					std::cout << " (and " << (tag->values.size() - 1) << " more)";
				}
				std::cout << "\n";
				if (++count >= 3) break;
			}
		}
	}

	std::cout << "\n=== Test completed successfully! ===\n";
	return 0;
}
