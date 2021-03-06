//this bodies file is autogenerated by tools/pivot_planner, don't edit this file, instead use pivotplanner to generate it.
enum BodyParts {
    BP_facing_east_head_000 = 0,
    BP_facing_north_head_000 = 1,
    BP_facing_south_head_000 = 2,
    BP_facing_west_head_000 = 3,
    BP_walking_east_body_000 = 4,
    BP_walking_east_body_001 = 5,
    BP_walking_north_body_000 = 6,
    BP_walking_north_body_001 = 7,
    BP_walking_south_body_000 = 8,
    BP_walking_south_body_001 = 9,
    BP_walking_west_body_000 = 10,
    BP_walking_west_body_001 = 11,
    BP_TOTAL = 12
};
internal void fill_generated_complex_values(FrameWithPivotAnchor* generated_frames ) {
    generated_frames[BP_facing_east_head_000] = (FrameWithPivotAnchor){111, 0, 11, 17,2, 2, 9, 15, 13, 19, 6, 17, 6, 2};
    generated_frames[BP_facing_north_head_000] = (FrameWithPivotAnchor){111, 17, 11, 17,2, 2, 9, 15, 13, 19, 6, 17, 6, 2};
    generated_frames[BP_facing_south_head_000] = (FrameWithPivotAnchor){111, 34, 11, 17,2, 2, 9, 15, 13, 19, 6, 17, 6, 2};
    generated_frames[BP_facing_west_head_000] = (FrameWithPivotAnchor){111, 51, 11, 17,2, 2, 9, 15, 13, 19, 6, 17, 6, 2};
    generated_frames[BP_walking_east_body_000] = (FrameWithPivotAnchor){21, 62, 19, 63,8, 37, 17, 61, 32, 100, 16, 97, 16, 37};
    generated_frames[BP_walking_east_body_001] = (FrameWithPivotAnchor){0, 0, 24, 62,8, 38, 22, 60, 32, 100, 16, 97, 16, 38};
    generated_frames[BP_walking_north_body_000] = (FrameWithPivotAnchor){48, 0, 21, 63,7, 37, 19, 61, 32, 100, 16, 97, 16, 37};
    generated_frames[BP_walking_north_body_001] = (FrameWithPivotAnchor){90, 0, 21, 62,7, 38, 19, 60, 32, 100, 16, 97, 16, 38};
    generated_frames[BP_walking_south_body_000] = (FrameWithPivotAnchor){69, 0, 21, 63,7, 37, 19, 61, 32, 100, 16, 97, 16, 37};
    generated_frames[BP_walking_south_body_001] = (FrameWithPivotAnchor){0, 62, 21, 62,7, 38, 19, 60, 32, 100, 16, 97, 16, 38};
    generated_frames[BP_walking_west_body_000] = (FrameWithPivotAnchor){90, 62, 19, 63,8, 37, 17, 61, 32, 100, 16, 97, 16, 37};
    generated_frames[BP_walking_west_body_001] = (FrameWithPivotAnchor){24, 0, 24, 62,3, 38, 22, 60, 32, 100, 16, 97, 16, 38};
};
