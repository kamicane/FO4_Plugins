# Calculate returned components when scrapping an object in Fallout 4

## MISC

If a MISC item has components in its form, and you scrap it, you get those components. No more, no less. It is a 1:1 correlation between what is in the form and what you get.

## WEAP and ARMO

For each OMOD in the armor / weapon, walk ALL the COBJs to find a matching Created Object.
Add the components for the COBJ calculation.

Check if the base object itself is also in a COBJ that has the ScrapRecipe keyword. Either in a FLST or by itself. If found, add the components for the COBJ calculation.

The number of components you get back depends on the ScrapScalar value of each CMPO. Non CMPO forms always have an hardcoded scrap scalar of 1.

## COBJ calculation

There are 5 scrap scalars used in the game, and each CMPO is assigned a scrap scalar. ScrapScalars are GLOBs.

| ScrapScalar | No Perk | Scrapper 1 | Scrapper 2 | Scrapper 3 |
| ----------- | ------- | ---------- | ---------- | ---------- |
| None        | 0       | 0          | 0          | 0          |
| SuperCommon | 1.0     | 1.0        | 1.0        | 1.0        |
| Full        | 0.5     | 0.5        | 0.5        | 0.5        |
| Uncommon    | 0       | 0.5        | 0.5        | 1.0        |
| Rare        | 0       | 0          | 0.5        | 0.5        |

- scrapper 1 sets Uncommon to 0.5
- scrapper 2 sets Rare to 0.5
- scrapper 3 sets Uncommon to 1.0 (DLC)

Having or not having the perk has no effect. On perk acquisition the GLOB ScrapScalars get stamped in your save game with new values. Calculations therefore must ignore presence of the perk, and read the GLOB value directly.

The scrapscalar values that get set when the player acquires the scrapper perks are hardcoded in 2 papyrus files. The only way to change them is to edit the base game scripts and re-acquire the perks.

The papyrus scripts are:

- Fragments:Quests:QF_PerksQuest_0004A09E
- DLC03:Fragments:Quests:QF_DLC03MQ00_01001B3E

The final calculation is a simple formula:

`NUM_RETURNED = FLOOR(0.5 * SCRAP_SCALAR * NUM_COMPONENTS)`

Always floor the total amounts. If a WEAP has 10 OMODS, collect totals for each component and floor the final result.

## Components vs their _scrap misc equivalent in scrap recipe COBJs

In the base game, COBJ scrap recipes are generally set to the _scrap MISC rather than the base component. Anything other than CMPOs will use a ScrapScalar value of 1.0

For instance, if a scrap recipe is set to 2x c_Adhesive, none will be returned, since c_Adhesive has a scrap scalar of 0 and `0.5 * 0 * 2` is 0. If the same scrap recipe is set to 2x c_Adhesive_scrap, 1 c_Adhesive_scrap will be returned, since `0.5 * 1 * 2` is 1.
