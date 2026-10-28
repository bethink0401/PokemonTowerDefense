# Map edit

Tool: built-in image_gen, precise-object-edit.
Input: forest_route.png (1774 x 887).
Output: forest_route_clean.png (1774 x 887).
Prompt: Remove ONLY the flat rectangular darker-green meadow deployment patches by replacing each with the same ordinary bright green lawn texture as immediately surrounding it. These are flat grass patches, NOT the tall grass bushes, trees or shrubs. Approximate patch bounds in original pixels: (272,280)-(329,326), (439,384)-(504,422), (613,409)-(667,449), (380,531)-(440,565), (504,526)-(551,563), (680,532)-(744,562), (731,260)-(769,293), (1150,286)-(1216,329), (1329,349)-(1382,389), (1152,466)-(1200,502), (991,583)-(1038,619), (868,659)-(918,695), (710,664)-(747,694). Blend away their darker rims too. Keep every road, bend, bridge, cave, building, tree, cliff, water feature, flower, sign and pixel art style unchanged and in exactly the same normalized positions. Keep original 2:1 map framing with no crop or added borders. Do NOT draw deployment markers: the game renders those itself. No new objects, labels, UI or watermarks.

Validation: road center samples retained; exact input/output dimensions; 13 logical deployment slots preserved. Markers are drawn by Qt/canvas, separately from the image.
