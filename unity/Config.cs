/**
 * Where we define the configuration for the project
 * 
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Config : MonoBehaviour
{
    public string player_name = "john";
    public string lobby = "DEFAULT";
    public int player_id = 0;
    volatile public int controller_sleep_ms = 50;
    public int heartbeat_sleep_ms = 3000;
}
