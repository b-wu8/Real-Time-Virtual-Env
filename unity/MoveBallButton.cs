using UnityEngine;
using System.Collections;

using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;

public class MoveBallButton : MonoBehaviour
{
    public Config config;
    public OculusClient oculus_client;
    public string temp_name, temp_lobby, message;

    // Start is called before the first frame update
    void Start()
    {
    }

    void OnGUI()
    {        
        GUIStyle style = new GUIStyle();
        style.alignment = TextAnchor.MiddleLeft;

        GUI.Box(new Rect(50, 325, 100, 20), message, style);
        GUI.Box(new Rect(50, 350, 100, 20), "Name", style);
        temp_name = GUI.TextField(new Rect(50, 375, 100, 20), temp_name);
        GUI.Box(new Rect(160, 350, 100, 20), "Lobby", style);
        temp_lobby = GUI.TextField(new Rect(160, 375, 100, 20), temp_lobby);
        UdpClient client = new UdpClient();
        if (GUI.Button(new Rect(270, 375, 80, 20), "JOIN"))
        {
            if (string.IsNullOrEmpty(temp_lobby) || string.IsNullOrEmpty(temp_name)) {
                message = "ERROR: name=\"" + temp_name + "\" lobby=\"" + temp_lobby + "\" variables cannot be empty!";
            } else if (temp_lobby == config.lobby && temp_name == config.player_name) {
                message = "MESSAGE: player \""+ temp_name + "\" is already in lobby \"" + temp_lobby + "\"";
            } else {
                message = "MESSAGE: player \"" + config.player_name + "\" left lobby \"" + config.lobby + 
                    "\" ... Player \"" + temp_name + "\" joined lobby \"" + temp_lobby;
                byte[] data = Encoding.UTF8.GetBytes(MessageTypes.FIN + " " + config.player_name + " " + config.lobby);
                client.Send(data, data.Length, oculus_client.server_endpoint);

                System.Threading.Thread.Sleep(100); // Make sure above message arrives first

                config.lobby = temp_lobby;
                config.player_name = temp_name;
                data = Encoding.UTF8.GetBytes(MessageTypes.SYN + " " + config.player_name + " " + config.lobby);
                oculus_client.receive_client.Send(data, data.Length, oculus_client.server_endpoint);

                temp_lobby = "";
                temp_name = "";
            }
        }
        /*
        if (GUI.Button(new Rect(360, 375, 80, 20), "LEAVE"))
        {
            se_handler.client.Send(data, data.Length, endpoint);
            se_handler.client.Send(data, data.Length, endpoint);
        }
        */
    }
}
