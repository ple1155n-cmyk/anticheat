package org.anticheat.bridge.network;

import lombok.Builder;
import lombok.Data;

import java.util.Map;

@Data
@Builder
public class MovementData {
    private String player; // UUID string
    private String action; // Action type (MOVE)
    private double x;
    private double y;
    private double z;
    private float yaw;
    private float pitch;
    private boolean onGround;
    private int ping;
    private String blockBelow;
    private Map<String, Integer> potions;
}
