using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System;
using Unity.XR.CoreUtils;

public class PlayerView : MonoBehaviour
{
    public Transform xr_rig_offset, camera_transform;
    public XROrigin xr_origin;
    public GameObject debug_canvas, debug_panel, debug_text, info_canvas, info_panel, info_text, minimap_canvas;
    public Dictionary<int, Avatar> avatars;
    public Config config;
    private GameObject plane;
    public volatile Dictionary<string, Vector3> interactable_2_loc;        // updated by oculus_client
    public Dictionary<string, int> interactable_2_owner_id;      // updated by oculus_client
    private Dictionary<string, GameObject> interactable_2_object;  // updated by player_view
    public int num_players;
    public bool is_show_debug;
    public bool is_show_birdview;
    public int MAX_PLAYERS_PER_LOBBY = AvatarColors.COLORS.Count;

    // Setting for action objects(serve as buttons)
    private Quaternion objects_spawn_rotation;
    private Vector3 server_objects_spawn_position;
    private Vector3 username_objects_spawn_position;
    private Vector3 action_objects_spawn_position;
    private Vector3 localscale_of_object;
    private float increment_of_position = 0.5f;

    public DebugDisplay debugger;

    public Vector3 POS_OFFSET = new Vector3(0f, 1.1176f, 0f);

    public void reset_xr_rig_offset()
    {
        xr_rig_offset.position = POS_OFFSET;
    }

    // Start is called before the first frame update
    void Start()
    {
        // init game objects
        //Debug.Log("Player View: Start");
        is_show_debug = false;    // by default, do not show debug canvas 
        is_show_birdview = false; // by default, do not show birdview canvas 

        reset_xr_rig_offset();
        interactable_2_loc = new Dictionary<string, Vector3>();
        interactable_2_object = new Dictionary<string, GameObject>();
        interactable_2_owner_id = new Dictionary<string, int>();

        // non-interactable init
        plane = GameObject.CreatePrimitive(PrimitiveType.Plane);
        plane.transform.position = new Vector3(0f, 0f, 0f);        // init location

        // debug canvas
        debug_canvas = new GameObject("Debug Info Canvas");
        debug_canvas.AddComponent<Canvas>();
        debug_canvas.GetComponent<Canvas>().renderMode = RenderMode.WorldSpace;
        debug_canvas.AddComponent<CanvasScaler>();
        debug_canvas.GetComponent<CanvasScaler>().dynamicPixelsPerUnit = 120;
        debug_canvas.AddComponent<GraphicRaycaster>();
        DebugDisplay debug_display = debug_canvas.AddComponent<DebugDisplay>();
  

        RectTransform debug_canvas_rectTransform = debug_canvas.GetComponent<RectTransform>();
        debug_canvas_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
        debug_canvas_rectTransform.sizeDelta = new Vector2(200f, 100f);
        debug_canvas_rectTransform.anchorMin = new Vector2(0f, 0f);
        debug_canvas_rectTransform.anchorMax = new Vector2(1f, 1f);
        debug_canvas_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        debug_canvas_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
        debug_canvas_rectTransform.localScale = new Vector3(0.05f, 0.05f, 0.05f);

        // debug panel
        debug_panel = new GameObject("Debug Info Panel");
        debug_panel.transform.SetParent(debug_canvas.transform);
        debug_panel.AddComponent<CanvasRenderer>();
        Image i_debug = debug_panel.AddComponent<Image>();
        i_debug.color = Color.black;

        // Provide Panel position and size using RectTransform
        RectTransform debug_panel_rectTransform = debug_panel.GetComponent<RectTransform>();
        debug_panel_rectTransform.anchorMin = new Vector2(0f, 0f);
        debug_panel_rectTransform.anchorMax = new Vector2(1f, 1f);
        debug_panel_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
        debug_panel_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        debug_panel_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
        debug_panel_rectTransform.localScale = new Vector3(1f, 1f, 1f);
        debug_panel_rectTransform.offsetMin = new Vector2(0f, 0f);
        debug_panel_rectTransform.offsetMax = new Vector2(0f, 0f);

        // Load the Arial font from the Unity Resources folder
        Font arial;
        arial = (Font)Resources.GetBuiltinResource(typeof(Font), "Arial.ttf");

        // debug text
        debug_text = new GameObject("Debug Info Text");
        debug_text.transform.SetParent(debug_panel.transform);
        debug_display.display = debug_text.AddComponent<Text>();

        Text debug_texts = debug_text.GetComponent<Text>();
        debug_texts.font = arial;
        debug_texts.text = "Debug display";
        debug_texts.fontSize = 5;
        debug_texts.color = Color.green;
        debug_texts.horizontalOverflow = HorizontalWrapMode.Overflow;
        debug_texts.verticalOverflow = VerticalWrapMode.Overflow;
        debug_texts.resizeTextForBestFit = true;

        // Provide debug Text position and size using RectTransform
        RectTransform debug_text_rectTransform;
        debug_text_rectTransform = debug_text.GetComponent<RectTransform>();
        debug_text_rectTransform.anchorMin = new Vector2(0f, 0f);
        debug_text_rectTransform.anchorMax = new Vector2(1f, 1f);
        debug_text_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
        debug_text_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        debug_text_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
        debug_text_rectTransform.localScale = new Vector3(1.2f, 1.2f, 1.2f);
        debug_text_rectTransform.offsetMin = new Vector2(20f, 20f);
        debug_text_rectTransform.offsetMax = new Vector2(-20f, -20f);

        debug_canvas.GetComponent<DebugDisplay>().display = debug_texts;

        arial = (Font)Resources.GetBuiltinResource(typeof(Font), "Arial.ttf");

        // server info canvas
        info_canvas = new GameObject("Server Info Canvas");
        info_canvas.AddComponent<Canvas>();
        info_canvas.GetComponent<Canvas>().renderMode = RenderMode.WorldSpace;
        info_canvas.AddComponent<CanvasScaler>();
        info_canvas.GetComponent<CanvasScaler>().dynamicPixelsPerUnit = 120;
        info_canvas.AddComponent<GraphicRaycaster>();

        RectTransform canvas_rectTransform = info_canvas.GetComponent<RectTransform>();
        canvas_rectTransform.localPosition = new Vector3(0f, 2.5f, 5f);
        canvas_rectTransform.sizeDelta = new Vector2(100f, 50f);
        canvas_rectTransform.anchorMin = new Vector2(0f, 0f);
        canvas_rectTransform.anchorMax = new Vector2(1f, 1f);
        canvas_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        canvas_rectTransform.rotation = Quaternion.Euler(0f, 180f, 0f);
        canvas_rectTransform.localScale = new Vector3(0.1f, 0.1f, 0.1f);

        // server info panel
        info_panel = new GameObject("Server Info Panel");
        info_panel.transform.SetParent(info_canvas.transform);
        info_panel.AddComponent<CanvasRenderer>();
        Image info_i = info_panel.AddComponent<Image>();
        info_i.color = Color.black;

        // Provide Panel position and size using RectTransform
        RectTransform panel_rectTransform = info_panel.GetComponent<RectTransform>();
        panel_rectTransform.anchorMin = new Vector2(0f, 0f);
        panel_rectTransform.anchorMax = new Vector2(1f, 1f);
        panel_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
        panel_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        panel_rectTransform.rotation = Quaternion.Euler(0f, 0f, 0f);
        panel_rectTransform.localScale = new Vector3(1f, 1f, 1f);
        panel_rectTransform.offsetMin = new Vector2(0f, 0f);
        panel_rectTransform.offsetMax = new Vector2(0f, 0f);

        // server info text
        string[] locations = { "SJC", "DEN", "LAX", "WAS", "CHI", "DFW", "ATL", "NYC" };
        string[] player_names = { "Yair", "Sahiti", "Bohan", "Brandon", "Daniel", "Evan", "Junjie", "Melody" };
        info_text = new GameObject("Server Info Text");
        info_text.transform.SetParent(info_panel.transform);
        info_text.AddComponent<Text>();

        // Load the Arial font from the Unity Resources folder
        // Font arial;
        // arial = (Font)Resources.GetBuiltinResource(typeof(Font), "Arial.ttf");
        Text text = info_text.GetComponent<Text>();
        text.font = arial;
        text.lineSpacing = 1.03f;

        // Format display text
        var sb = new System.Text.StringBuilder();
        sb.Append("\n");
        sb.Append(String.Format("{0,15} {1,15}\n", "LOCATION", "PLAYER NAMES"));
        for (int index = 0; index < locations.Length; index++)
        {
            if (index == 0)
            {
                sb.Append(String.Format("{0,15} {1,25:N0}\t\t\t JOIN\n", locations[index], player_names[index]));
                continue;
            }
            //else if (index == 1)
            //{
            //    sb.Append(String.Format("{0,15} {1,25:N0}\t\t\t LEAVE\n", locations[index], player_names[index]));
            //    continue;
            //}
            else if (index == 1)
            {
                sb.Append(String.Format("{0,15} {1,25:N0}\t\t MINIMAP\n", locations[index], player_names[index]));
                continue;
            }
            else if (index == 2)
            {
                sb.Append(String.Format("{0,15} {1,25:N0}\t\t STATISTICS\n", locations[index], player_names[index]));
                continue;
            } 
            sb.Append(String.Format("{0,15} {1,25:N0}\n", locations[index], player_names[index]));
        }
        text.text = sb.ToString();
        text.fontSize = 12;
        text.color = Color.green;
        text.horizontalOverflow = HorizontalWrapMode.Overflow;
        text.verticalOverflow = VerticalWrapMode.Overflow;

        // Provide Text position and size using RectTransform
        RectTransform text_rectTransform;
        text_rectTransform = info_text.GetComponent<RectTransform>();
        text_rectTransform.anchorMin = new Vector2(0f, 0f);
        text_rectTransform.anchorMax = new Vector2(1f, 1f);
        text_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
        text_rectTransform.pivot = new Vector2(0.5f, 0.5f);
        text_rectTransform.rotation = Quaternion.Euler(0f, 0f, 0f);
        text_rectTransform.localScale = new Vector3(1f, 1f, 1f);
        text_rectTransform.offsetMin = new Vector2(0f, 0f);
        text_rectTransform.offsetMax = new Vector2(0f, 0f);

        avatars = new Dictionary<int, Avatar>();

        server_objects_spawn_position = new Vector3(-4f, 3.8f, 5.25f); // server object spawned position
        username_objects_spawn_position = new Vector3(-1f, 3.8f, 5.25f);
        action_objects_spawn_position = new Vector3(4.5f, 3.8f, 5.25f); 
        objects_spawn_rotation = Quaternion.Euler(-90f, 0f, 0f);    // button objects rotation
        localscale_of_object = new Vector3(0.3f, 0.3f, 0.3f);
    }

    private PrimitiveType get_object_type(string item_name)
    {
        string[] item_name_strings = item_name.Split('_');
        string type_string = item_name_strings[item_name_strings.Length - 1];
        switch (type_string)
        {
            case "sphere": return PrimitiveType.Sphere;
            case "plance": return PrimitiveType.Plane;
            case "cylinder": return PrimitiveType.Cylinder;
            case "quad": return PrimitiveType.Quad;
            case "capsule": return PrimitiveType.Capsule;
            default: return PrimitiveType.Cube;
        }
    }

    private GameObject create_object(string item_name)
    {
        GameObject newObject = GameObject.CreatePrimitive(get_object_type(item_name));
        newObject.transform.position = new Vector3(0f, -1f, 0f);      // init location
        return newObject;
    }

    // Update is called once per frame
    public void Update()
    {   try
        {
            // Create new game objects or update objects position/color
            foreach (KeyValuePair<string, Vector3> kv_item_name_2_loc in interactable_2_loc)
            {
                if (!interactable_2_object.ContainsKey(kv_item_name_2_loc.Key)) // if this object does not exist
                {
                    interactable_2_object.Add(kv_item_name_2_loc.Key, create_object(kv_item_name_2_loc.Key));
                }
                interactable_2_object[kv_item_name_2_loc.Key].transform.position = kv_item_name_2_loc.Value; // update position

                string item_name = kv_item_name_2_loc.Key;

                if (!interactable_2_owner_id.ContainsKey(item_name))
                    continue;

                // update object colors according to the owner
                int owner_id = interactable_2_owner_id[item_name];
                if (owner_id == 0)  // if owner exited the game or no user owns the object
                {
                    interactable_2_object[item_name].GetComponent<Renderer>().material    // change color to grey
                    .SetColor("_Color", Color.gray);
                }
                else
                {
                    interactable_2_object[item_name].GetComponent<Renderer>().material    // change color to owner's avatar color
                        .SetColor("_Color", AvatarColors.COLORS[owner_id % MAX_PLAYERS_PER_LOBBY]);
                }
            }


            // Check if any avatars were marked for destruction
            bool destroyed_avatar = false; ;
            foreach (KeyValuePair<int, Avatar> kv_avatar in avatars)
            {
                if (!kv_avatar.Value.to_be_destroyed)
                    kv_avatar.Value.render();
                else
                {
                    kv_avatar.Value.destroy();
                    destroyed_avatar = true;
                }
            }

            // If there were any destroyed avatars, remvoe them
            if (destroyed_avatar)
            {
                Dictionary<int, Avatar> temp_avatars = new Dictionary<int, Avatar>();
                foreach (KeyValuePair<int, Avatar> kv_avatar in avatars)
                    if (!kv_avatar.Value.to_be_destroyed)
                        temp_avatars.Add(kv_avatar.Key, kv_avatar.Value);
                avatars = temp_avatars;
            }

            // Move the players camera rig
            if (avatars.ContainsKey(config.player_id))
            {
                Avatar main_avatar = avatars[config.player_id];
                xr_rig_offset.position = main_avatar.offset;
                POS_OFFSET = main_avatar.offset;
            }

            // Check if we should change state of debug canvas
            if (is_show_debug != debug_canvas.activeSelf)
                debug_canvas.SetActive(is_show_debug);


            // Check if we should change state of debug canvas
            if (is_show_birdview != minimap_canvas.activeSelf)
                minimap_canvas.SetActive(is_show_birdview);
        }
        catch (Exception e)
        {
                
        }
    }

    public GameObject CreateServerObjects()
    {
        GameObject server_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        server_object.transform.position = server_objects_spawn_position;
        server_object.transform.localScale = localscale_of_object;
        server_object.transform.rotation = objects_spawn_rotation;
        server_objects_spawn_position.y -= increment_of_position;

        return server_object;
    }

    public GameObject CreateUserNameObjects()
    {
        GameObject username_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        username_object.transform.position = username_objects_spawn_position;
        username_object.transform.localScale = localscale_of_object;
        username_object.transform.rotation = objects_spawn_rotation;
        username_objects_spawn_position.y -= increment_of_position;

        return username_object;
    }

    public GameObject CreateJoinBtnObject()
    {
        GameObject confirm_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        confirm_object.transform.position = action_objects_spawn_position;
        confirm_object.transform.localScale = localscale_of_object;
        confirm_object.transform.rotation = objects_spawn_rotation;
        action_objects_spawn_position.y -= increment_of_position;
        confirm_object.name = "JoinBtnObject";
        return confirm_object;
    }

    public GameObject CreateDebugBtnObject()
    {
        GameObject confirm_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        confirm_object.transform.position = action_objects_spawn_position;
        confirm_object.transform.localScale = localscale_of_object;
        confirm_object.transform.rotation = objects_spawn_rotation;
        action_objects_spawn_position.y -= increment_of_position;
        confirm_object.name = "DebugBtnObject";
        return confirm_object;
    }


    public GameObject CreateMinimapBtnObject()
    {
        GameObject confirm_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        confirm_object.transform.position = action_objects_spawn_position;
        confirm_object.transform.localScale = localscale_of_object;
        confirm_object.transform.rotation = objects_spawn_rotation;
        action_objects_spawn_position.y -= increment_of_position;
        confirm_object.name = "MinimapBtnObject";
        return confirm_object;
    }


    //public GameObject CreateLeaveBtnObject()
    //{
    //    GameObject confirm_object = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
    //    confirm_object.transform.position = action_objects_spawn_position;
    //    confirm_object.transform.localScale = localscale_of_object;
    //    confirm_object.transform.rotation = objects_spawn_rotation;
    //    action_objects_spawn_position.y -= increment_of_position;
    //    confirm_object.name = "LeaveBtnObject";
    //    return confirm_object;
    //}

    public bool is_interactable(GameObject game_object)
    {
        return interactable_2_object.ContainsValue(game_object);
    }

    public string get_interactable_name(GameObject game_object)
    {
        foreach (KeyValuePair<string, GameObject> kv_interactable_2_object in interactable_2_object)
        {
            if (kv_interactable_2_object.Value.Equals(game_object))
            {
                return kv_interactable_2_object.Key;
            }
        }
        return "";
    }

    public void destroy_all_avatar_objects() {
        foreach (KeyValuePair<int, Avatar> kv_player_id_2_avatar in avatars)
        {
            kv_player_id_2_avatar.Value.destroy();
        }
        avatars.Clear();
    }
    
}
