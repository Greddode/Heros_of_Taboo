# Heroes of Taboo — Version History

This file preserves the pre-refactoring changelog entries (ALPHA-0.0.4 through ALPHA-0.0.8).
For recent releases, see [changelog.md](./changelog.md).

---

## Historical (pre-refactor, preserved for reference)


## ALPHA-0.0.8 — Equipment & Stat System

### For Players

- **Equipment system** — 17 equipment types across 3 categories (Armor, Weapon, Accessory) with 5 equip slots (Head, Chest, Weapon, Off-hand, Accessory). Each item provides stat bonuses (ATK, DEF, STR, DEX, INT, CON, LCK). Two-handed weapons (War Hammer) block the off-hand slot.
- **Inventory UI tabs** — inventory screen now has 3 tabs (Inventory / Equipment / Stats) switchable with Q/E. Inventory tab shows potions and unequipped gear; Equipment tab shows equipped items with Unequip/Drop/Back actions; Stats tab displays base stats, derived stats, and unspent stat points.
- **Stat point allocation** — gain +2 stat points per level. Allocate to STR (damage), DEX (dodge), MGC (potion healing), CON (max HP), or LCK (crit chance, loot rarity). Max HP is now derived from CON (30 + CON x 5).
- **Loot system overhaul** — unified loot table with 4 rarity tiers: Common (Tier 1), Uncommon (Tier 2), Rare (Tier 3, floor 4+), Legendary (Tier 4, floor 6+). Luck and floor depth boost higher tier drops. Equipment drops from monsters on death (20% base + LCK x 2, max 50%).
- **Equipment on the ground** — equipment items now spawn on dungeon floors with their own sprites. Stacked items show a loot pile icon with quantity badge. Click to inspect.
- **Monster equipment drops** — defeating a monster has a chance to drop a random accessory (filtered by floor availability).
- **Level-up notification** — gold "LEVEL UP!" overlay with "+3 Stat Points!" subtitle, fades out over 3 seconds. Level-up sound effect added.
- **Potion heal scales with INT** — potion healing is now `base% x (1 + INT x 0.02)`, making Intelligence stat meaningful for sustain.
- **Potion rarity on ground** — floor-placed potions now roll randomly (50% small, 30% medium, 20% large) instead of being floor-locked.
- **GUI Scale setting** — adjustable UI scale (1.0x to 4.0x) in the Settings menu. All UI elements scale proportionally.
- **Updated controls screen** — documents Q/E tab switching, sprint controls, mouse click inspect, and equipment management.
- **Scrollable menus** — main menu, story, credits, and controls screens now scroll when content exceeds the viewport.
- **Starting equipment** — player now starts with a Survival Knife (+2 ATK) instead of bare fists.

### For Developers

See git history for detailed developer notes from ALPHA-0.0.8 and earlier.

---

## ALPHA-0.0.7 — Ranged Combat & AI Rebalance

See git history.

---

## ALPHA-0.0.6 — Rebalance & Inventory Update

See git history.

---

## ALPHA-0.0.5 — Story Board & Floors Update

See git history.

---

## ALPHA-0.0.4 — Music Update

See git history.
