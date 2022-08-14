/* 
 * Declares the classes used in identifying data 
 * about a player using an Oculus device.
 */

using System;
using UnityEngine;
using System.Collections.Generic;

public static class AvatarColors {
    static string[] color_strings = {
            "Yellow",
            "Cyan",
            "Pink",
            "Green",
            "Red",
            "Blue",
            "Orange",
            "Purple"
    };

    public static IReadOnlyList<Color> COLORS = new List<Color>(new Color[] {
        new Color(1f, 1f, 0f),  // Yellow
        new Color(0f, 1f, 1f),  // Cyan
        new Color(1f, 0f, 0.5f),  // Pink
        new Color(0f, 1f, 0f),  // Green
        new Color(1f, 0f, 0f),  // Red
        new Color(0f, 0f, 1f),  // Blue
        new Color(1f, 0.5f, 0f),  // Orange
        new Color(0.5f,0f,1f),  // Purple
    });
    public static string get_color_string(int player_id)
    {
        return color_strings[player_id % COLORS.Count];
    }
}

public class Avatar {
    private int avatar_id; // the same as player id
    private Color color;
    public string timestamp;
    public Headset headset_controller;
    public LeftHandController left_controller;
    public RightHandController right_controller;
    public Vector3 offset;
    public GameObject head, left_hand, right_hand;
    public bool is_created, to_be_destroyed;
    private bool is_current_player = false;

    public Avatar(int avatar_id,  bool is_current_player = false) 
    {
        // Debug.Log("Created new avatar " + avatar_id);
        this.offset = new Vector3(0f, 1.1176f, 0f);  // TODO: Remove this (offset is set by server)
        this.color = AvatarColors.COLORS[avatar_id % AvatarColors.COLORS.Count];
        this.avatar_id = avatar_id;
        this.is_created = this.to_be_destroyed = false;
        this.is_current_player = is_current_player;
    }
    
    public Avatar(Avatar other)
    {
        // Debug.Log("Created new avatar in Avatar(Avatar other)");
        this.avatar_id = other.avatar_id;
        this.is_created = false;
        this.headset_controller = (Headset) Headset.deep_copy(other.headset_controller);
        this.left_controller = (LeftHandController) LeftHandController.deep_copy(other.left_controller);
        this.right_controller = (RightHandController) RightHandController.deep_copy(other.right_controller);
    }

    public void update(ADS.ContinuousData data)
    {
        this.headset_controller = new Headset(
            new Vector3(
                data.Headset.Position.X,
                data.Headset.Position.Y,
                data.Headset.Position.Z
                ),
            new Quaternion
            (
                data.Headset.Rotation.X,
                data.Headset.Rotation.Y,
                data.Headset.Rotation.Z,
                data.Headset.Rotation.W
            ));
        this.left_controller = new LeftHandController(
            new Vector3(
                data.LeftController.Position.X,
                data.LeftController.Position.Y,
                data.LeftController.Position.Z
                ),
            new Quaternion
            (
                data.LeftController.Rotation.X,
                data.LeftController.Rotation.Y,
                data.LeftController.Rotation.Z,
                data.LeftController.Rotation.W
            ));
        this.right_controller = new RightHandController(
            new Vector3(
                data.RightController.Position.X,
                data.RightController.Position.Y,
                data.RightController.Position.Z
                ),
            new Quaternion
            (
                data.RightController.Rotation.X,
                data.RightController.Rotation.Y,
                data.RightController.Rotation.Z,
                data.RightController.Rotation.W
            ));
        this.offset = new Vector3(
                data.PlayerOffset.X,
                data.PlayerOffset.Y,
                data.PlayerOffset.Z
            );
        // Debug.Log("Updating avatar of player " + avatar_id + " head : " + headset_controller.to_string());
        // Debug.Log("Updating avatar of player " + avatar_id + " left_controller : " + left_controller.to_string());
        // Debug.Log("Updating avatar of player " + avatar_id + " right_controller : " + right_controller.to_string());
    }
    
    public static Vector3 StringToVec3(string str_x, string str_y, string str_z){
        return new Vector3(float.Parse(str_x), float.Parse(str_y), float.Parse(str_z));
    }

    public static Vector2 StringToVec2(string str_x, string str_y){
        return new Vector2(float.Parse(str_x), float.Parse(str_y));
    }

    public static Quaternion StringToQuat(string str_x, string str_y, string str_z, string str_w){
        return new Quaternion(float.Parse(str_x), float.Parse(str_y), float.Parse(str_z), float.Parse(str_w));
    }

    public void create() {
        if (is_created) {
            // Debug.Log("Avatar/ already created");
            return;
        }

        head = GameObject.CreatePrimitive(PrimitiveType.Cube);
        head.transform.localScale = new Vector3(0.4f, 0.4f, 0.4f);
        head.transform.position = headset_controller.position + offset;
        head.transform.rotation = headset_controller.rotation;
        head.GetComponent<Renderer>().material.SetColor("_Color", color);

        if (is_current_player)
            head.SetActive(false);

        left_hand = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        left_hand.transform.localScale = new Vector3(0.15f, 0.15f, 0.15f);
        left_hand.transform.position = left_controller.position + offset;
        left_hand.transform.rotation = left_controller.rotation;
        left_hand.GetComponent<Renderer>().material.SetColor("_Color", color);

        right_hand = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        right_hand.transform.localScale = new Vector3(0.15f, 0.15f, 0.15f);
        right_hand.transform.position = right_controller.position + offset;
        right_hand.transform.rotation = right_controller.rotation;
        right_hand.GetComponent<Renderer>().material.SetColor("_Color", color);

        is_created = true;
    }

    public void destroy() {
        if (!is_created) {
            // Debug.Log("No avatar to destroy");
            return;
        }

        GameObject.Destroy(head);
        GameObject.Destroy(left_hand);
        GameObject.Destroy(right_hand);
        // Debug.Log("Destroyed avatar for " + avatar_id);
    }

    public void render() {
        if (!is_created) {
            create();
            return;
        }

        head.transform.position = headset_controller.position + offset;
        head.transform.rotation = headset_controller.rotation;

        left_hand.transform.position = left_controller.position + offset;
        left_hand.transform.rotation = left_controller.rotation;

        right_hand.transform.position = right_controller.position + offset;
        right_hand.transform.rotation = right_controller.rotation;



        // Debug.Log("right hand transform : position" + right_hand.transform.position.ToString());
    }
}
