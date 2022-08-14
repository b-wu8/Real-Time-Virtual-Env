using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Threading;
using Google.Protobuf;
using UnityEngine;

/*
 * Types to classify what kind of message is being sent.
 */
static class MessageTypes
{
    public const int SYN = 1;
    public const int INPUT = 2;
    public const int FIN = 3;
    public const int HEARTBEAT = 4;
    public const int DATA = 5;
    public const int LEAVE = 6;
    public const int TEST = 7;
}

static class ServerNames
{
    public static string get_server_name_by_id(int id) {
        switch (id) {
            case 1: return "SJC";
            case 3: return "DEN";
            case 11: return "LAX";
            case 12: return "WAS";
            case 13: return "CHI";
            case 14: return "DFW";
            case 15: return "ATL";
            case 16: return "NYC";
            default: return "UNKNOWN";
        }
    }
}

static class OculusButtons
{
    public const string RIGHT_PRIMARY_BUTTON = "RightPrimaryButton";  // A
    public const string RIGHT_SECODNARY_BUTTON = "RightSecondaryButton";  // B
    public const string LEFT_PRIMARY_BUTTON = "LeftPrimaryButton";  // X
    public const string LEFT_SECONDARY_BUTTON = "LeftSecondaryButton";  // Y
    public const string RIGHT_TRIGGER_BUTTON = "RightTriggerButton"; 
    public const string LEFT_TRIGGER_BUTTON = "LeftTriggerButton"; 
}

static class RPCs
{
    public const string HAPTICS = "HAPTICS";
    public const string LEFT_PRIMARY_BUTTON_DOWN = "LEFT_PRIMARY_BUTTON_DOWN";
    public const string RIGHT_PRIMARY_BUTTON_DOWN = "RIGHT_PRIMARY_BUTTON_DOWN";
    public const string LEFT_SECONDARY_BUTTON_DOWN = "LEFT_SECONDARY_BUTTON_DOWN";
    public const string RIGHT_SECONDARY_BUTTON_DOWN = "RIGHT_SECONDARY_BUTTON_DOWN";
    public const string LEFT_PRIMARY_BUTTON_UP = "LEFT_PRIMARY_BUTTON_UP";
    public const string RIGHT_PRIMARY_BUTTON_UP = "RIGHT_PRIMARY_BUTTON_UP";
    public const string LEFT_SECONDARY_BUTTON_UP = "LEFT_SECONDARY_BUTTON_UP";
    public const string RIGHT_SECONDARY_BUTTON_UP = "RIGHT_SECONDARY_BUTTON_UP";
}

/*
// Event driven code which we no longer use
public class ButtonEvent : UnityEvent<bool> { }
private ButtonEvent left_primary_button_event;
left_primary_button_event = new ButtonEvent();
left_primary_button_event.AddListener(HandleLeftPrimaryButton);
left_primary_button_event.Invoke(is_pushed);
*/

public class OculusClient : MonoBehaviour
{
    public Config config;
    public PlayerView view;
    public DeviceInfoWatcher device_watcher;
    private string last_packet;
    private int discrete_timeout_ms;
    private Dictionary<string, bool> current_discrete_state;
    public UdpClient receive_client, heartbeat_client, continuous_input_client, discrete_input_client;
    private Thread receive_thread, heartbeat_thread, continuous_input_thread, discrete_input_thread;
    private DateTime ping_start_time;
    public IPEndPoint server_endpoint;
    public static string inputText = "";
    private Dictionary<int, string> players_presence;
    private Dictionary<GameObject, (string, string)> object_2_ingest_server;
    private Dictionary<GameObject, string> object_2_username;
    GameObject join_btn_object, debug_btn_object, minimap_btn_object; //leave_btn_object;
    private int DEFAULT_SERVER_ID = 0;
    private int ingest_server_id = 0;
    private string ingest_server_ip;
    private int ingest_server_port = 4500;
    private GameObject action_object;
    private int frame_count = 0;
    private int color_fade_frames = 30;
    private bool is_server_set = false;
    private bool is_username_set = false;
    private string ingest_server_name = "";
    private float tickrate_factor = 0.5f;
    private HashSet<string> item_claimed_by_current_user;
    private string RPC_UNCLAIM_STR_PREFIX = "unclaim__";
    private string RPC_CLAIM_STR_PREFIX = "claim__";
    private string RPC_PING_STR_PREFIX = "ping__";
    private string RPC_RAND_STR_PREFIX = "rand__";
    private string RPC_STATS_STR_PREFIX = "stats__";
    private Dictionary<string, (UInt64, UInt64)> data_center_2_deltas; // datacenter("SJC" to (rec delta, handle delta))
    private Dictionary<int, (string, string)> player_2_infos; // player_id to (player_name, datacenter)

    // For Statistics
    // Instantiate random number generator using system-supplied value as seed.
    System.Random rand = new System.Random();
    private Dictionary<int, long> rand_2_start_unix_time_millisec;
    private Dictionary<int, UInt32> rand_2_hash;
    private UInt32 last_rand_hash;
    int random_num_length = 10;
    string random_str_format = "D10"; // same random_num_length
    int MAX_RANDOM_NUM = Int32.MaxValue;
    long cumulated_server_client_ping_ms = 0;
    long cumulated_request_rtt_ms = 0;
    int server_client_ping_count = 0;
    int request_rtt_count = 0;
    long MAX_CUMULATED_COUNT = 10000;

    public void Start()
    {
        receive_client = new UdpClient();
        receive_thread = new Thread(new ThreadStart(ReceiveData));
        receive_thread.IsBackground = true;
        receive_thread.Start();

        heartbeat_client = new UdpClient();
        heartbeat_thread = new Thread(new ThreadStart(Heartbeat));
        heartbeat_thread.IsBackground = true;
        heartbeat_thread.Start();

        continuous_input_client = new UdpClient();
        continuous_input_thread = new Thread(new ThreadStart(SendContinuousData));
        continuous_input_thread.IsBackground = true;
        continuous_input_thread.Start();

        discrete_timeout_ms = 10;
        current_discrete_state = new Dictionary<string, bool>();
        // Use reflection to set all discrete state buttons to false
        foreach (FieldInfo field in typeof(OculusButtons).GetFields()) 
            current_discrete_state.Add(field.GetValue(null).ToString(), false);        
        discrete_input_client = new UdpClient();
        discrete_input_thread = new Thread(new ThreadStart(SendDiscreteData));
        discrete_input_thread.IsBackground = true;
        discrete_input_thread.Start();

        players_presence = new Dictionary<int, string>();

        //available ingest servers
        object_2_ingest_server = new Dictionary<GameObject, (string, string)>();

        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.21", "SJC"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.23", "DEN"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.31", "LAX"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.32", "WAS"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.33", "CHI"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.34", "DFW"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.35", "ATL"));
        object_2_ingest_server.Add(view.CreateServerObjects(), ("128.220.221.36", "NYC"));

        object_2_username = new Dictionary<GameObject, string>();
        object_2_username.Add(view.CreateUserNameObjects(), "Yair");
        object_2_username.Add(view.CreateUserNameObjects(), "Sahiti");
        object_2_username.Add(view.CreateUserNameObjects(), "Bohan");
        object_2_username.Add(view.CreateUserNameObjects(), "Brandon");
        object_2_username.Add(view.CreateUserNameObjects(), "Daniel");
        object_2_username.Add(view.CreateUserNameObjects(), "Evan");
        object_2_username.Add(view.CreateUserNameObjects(), "Junjie");
        object_2_username.Add(view.CreateUserNameObjects(), "Melody");

        join_btn_object = view.CreateJoinBtnObject();
        //leave_btn_object = view.CreateLeaveBtnObject();
        minimap_btn_object = view.CreateMinimapBtnObject();
        debug_btn_object = view.CreateDebugBtnObject();

        //TODO: delete this
        //ingest_server_ip = "128.220.221.35";
        //ADS.SynRequest syn_request = new ADS.SynRequest
        //{
        //    PlayerName = config.player_name,
        //    LobbyName = config.lobby,
        //    IngestServerId = 0,
        //    ServerName = ingest_server_name
        //};
        //byte[] temp = GetMessageByteArray(ADS.Type.Syn, syn_request.ToByteString());
        //server_endpoint = new IPEndPoint(IPAddress.Parse(ingest_server_ip), ingest_server_port);
        //receive_client.Send(temp, temp.Length, server_endpoint);

        send_to_all_servers(); //opens NAT for all servers

        item_claimed_by_current_user = new HashSet<string>();
        rand_2_start_unix_time_millisec = new Dictionary<int, long>();
        rand_2_hash = new Dictionary<int, uint>();

        data_center_2_deltas = new Dictionary<string, (UInt64, UInt64)>();
        player_2_infos = new Dictionary<int, (string, string)>();

        Debug.Log("Server Name: 1");
        Debug.Log("Player Name: 1");
        Debug.Log("Client-Server Ping: 1"); //TODO: Client-Server Ping * 2
        //Debug.Log("Request RTT: 1"); //TODO: Request RTT * 2
    }

    void send_to_all_servers()
    {   foreach(KeyValuePair<GameObject, (string,string)> kv_object_2_ip_port in object_2_ingest_server)
        {
            string ip_str = kv_object_2_ip_port.Value.Item1;
            IPEndPoint temp_endpoint = new IPEndPoint(IPAddress.Parse(ip_str), ingest_server_port);
            byte[] data = GetMessageByteArray(ADS.Type.Unknown);
            receive_client.Send(data, data.Length, temp_endpoint);
        }
    }

    void log_statistics() {

        Debug.Log("Server Name: " + ServerNames.get_server_name_by_id(ingest_server_id));
        Debug.Log("Player Name: " + config.player_name);

        string debug_str = "\nPlayers:\n";
        debug_str  += ("name" + " ( color )" +
                    " - " +
                    " " + "server" +
                    "   " + "rcv delta" +
                    "  " + "handle delta" +
                    "\n"
                    );
        foreach (KeyValuePair<int, (string, string)> kv_player_2_infos in player_2_infos)
        {
            int player_id = kv_player_2_infos.Key;
            string player_name = kv_player_2_infos.Value.Item1;
            string data_center = kv_player_2_infos.Value.Item2;
            if (data_center_2_deltas.ContainsKey(data_center))
            {
                debug_str += ( player_name + " ( " + AvatarColors.get_color_string(player_id) + " )" +
                    " - " +
                    " " + data_center +
                    "      " + (((double)data_center_2_deltas[data_center].Item1/ (double)1000)).ToString("N1") +
                    "ms      " + (((double)data_center_2_deltas[data_center].Item2/ (double)1000)).ToString("N1") +
                    "ms \n"
                    );
            }
        }
        Debug.Log(debug_str);
    }

    void OnGUI()
    {
        Rect rectObj = new Rect(40, 10, 200, 400);
        GUIStyle style = new GUIStyle();
        style.alignment = TextAnchor.UpperLeft;
        GUI.Box(rectObj, "Last Packet: \n" + last_packet, style);
    }

    void OnApplicationQuit()
    {   
        receive_thread.Abort();

        continuous_input_thread.Abort();
        discrete_input_thread.Abort();

        byte[] data = GetMessageByteArray(ADS.Type.Fin);
        heartbeat_client.Send(data, data.Length, server_endpoint);
        heartbeat_thread.Abort();
    }

    private void HandleWorldMessage(ByteString data) {
        ADS.WorldResponse world_data = ADS.WorldResponse.Parser.ParseFrom(data);

        view.num_players = world_data.Avatars.Count;

        // Debug.Log("# players in the world: " + world_data.Avatars.Count);

        // parsing all the user avatars
        for (int i = 0; i < world_data.Avatars.Count; i++)
        {
            int player_id = world_data.Avatars[i].PlayerId;
            if (view.avatars.ContainsKey(player_id))
            {
                //Debug.Log("Update avatar for player : " + player_id) ;
                view.avatars[player_id].update(world_data.Avatars[i].Data);
            }
            else
            {
                //Debug.Log("New player id " + player_id);
                view.avatars.Add(player_id, new Avatar(player_id, config.player_id == player_id));
                view.avatars[player_id].update(world_data.Avatars[i].Data);
            }
        }

        // parsing all the interactables
        for (int i = 0; i < world_data.Items.Count; i++)
        {
            int owner_id = world_data.Items[i].OwnerId;
            string item_name = world_data.Items[i].ItemName;
            Vector3 updated_loc = new Vector3(world_data.Items[i].Position.X, world_data.Items[i].Position.Y, world_data.Items[i].Position.Z);

            if (!view.interactable_2_owner_id.ContainsKey(item_name)) // add new item
            {
                view.interactable_2_owner_id.Add(item_name, owner_id);
                view.interactable_2_loc.Add(item_name, updated_loc);
            }
            else
            {
                view.interactable_2_owner_id[item_name] = owner_id;
                view.interactable_2_loc[item_name] = updated_loc;
            }

            if (owner_id == config.player_id)  // newly claimed
            {
                item_claimed_by_current_user.Add(item_name);
            } else if (item_claimed_by_current_user.Contains(item_name)) // if claimed by others
            {
                item_claimed_by_current_user.Remove(item_name);
            }

        }

    }

    private void log_server_client_ping(long new_ping) {
        cumulated_server_client_ping_ms += new_ping;
        server_client_ping_count += 1;
        Debug.Log("Client-Server Ping: " + "last( " + new_ping + ".0 ms )" + "       average( " + ((double)(cumulated_server_client_ping_ms) / server_client_ping_count).ToString("N1") + " ms )");
        if (server_client_ping_count >= MAX_CUMULATED_COUNT)
        {
            cumulated_server_client_ping_ms = 0;
            server_client_ping_count = 0;
        }
    }

    private void log_request_rtt(long new_rtt)
    {
        cumulated_request_rtt_ms += new_rtt;
        request_rtt_count += 1;
        //Debug.Log("Request RTT: " + "        last( " + new_rtt + ".0 ms )" + "       average( " + ((double)(cumulated_request_rtt_ms) / request_rtt_count).ToString("N1") + " ms )");
        if (request_rtt_count >= MAX_CUMULATED_COUNT)
        {
            cumulated_request_rtt_ms = 0;
            request_rtt_count = 0;
        }
    }

    private void HandleHeartbeatMessage(ByteString data) {
        DateTime ping_end_time = DateTime.Now;
        TimeSpan ping_time = ping_end_time.Subtract(ping_start_time);

        log_server_client_ping(ping_time.Milliseconds);

        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc,
            (
                new ADS.RpcRequest
                {
                    Rpc = RPC_PING_STR_PREFIX + (ping_time.Milliseconds * 1000).ToString(),
                    InitiatorId = config.player_id
                }
            ).ToByteString()
        );
        heartbeat_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void HandleSynMessage(ByteString data)
    {
        ADS.SynResponse syn_response = ADS.SynResponse.Parser.ParseFrom(data);
        config.player_id = syn_response.ClientId;
        //Debug.Log("set config.player_id to " + config.player_id);
        config.player_name = syn_response.PlayerName; // server-validated player name
        config.lobby = syn_response.LobbyName; // server-validated lobby name
        config.controller_sleep_ms = (int)((float)syn_response.ServerTickDelayMs * tickrate_factor);
        view.MAX_PLAYERS_PER_LOBBY = syn_response.MaxPlayersPerLobby;
        // Debug.Log("Ingest Server [ IP PORT ID ]" + DebugDisplay.SPLITTER + 
        // "[ " + syn_response.IngestServerIp + " " + syn_response.IngestServerPort + " " + syn_response.IngestServerId + " ]");
        //server_endpoint = new IPEndPoint(IPAddress.Parse(syn_response.IgnestServerIp), syn_response.IngestServerPort);
        ingest_server_id = syn_response.IngestServerId;
    }

    private void HandleRPC(string rpc)
    {
        switch(rpc)
        {
            case RPCs.HAPTICS:
                // vibration on both controllers
                device_watcher.RumbleLeftController();
                break;
            default:
                if (rpc.StartsWith(RPC_RAND_STR_PREFIX))
                {

                    int rcv_rand_num = 0;
                    try
                    {
                        rcv_rand_num = Int32.Parse(rpc.Substring(RPC_RAND_STR_PREFIX.Length, random_num_length));
                    }
                    catch (FormatException e)
                    {
                        Debug.LogException(e);
                    }

                    if (rand_2_start_unix_time_millisec.ContainsKey(rcv_rand_num) && rand_2_hash.ContainsKey(rcv_rand_num))
                    {
                        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc,
                           (
                               new ADS.RpcRequest
                               {
                                   Rpc = RPC_STATS_STR_PREFIX 
                                   + (rand_2_start_unix_time_millisec[rcv_rand_num] * 1000).ToString()
                                   + "_"
                                   + (((DateTimeOffset)DateTime.Now).ToUnixTimeMilliseconds()*1000).ToString()
                                   + "_"
                                   + rand_2_hash[rcv_rand_num]
                                   ,
                                   InitiatorId = config.player_id
                               }
                           ).ToByteString()
                        );;
                        long request_rtt = ((DateTimeOffset)DateTime.Now).ToUnixTimeMilliseconds() - rand_2_start_unix_time_millisec[rcv_rand_num];
                        log_request_rtt(request_rtt);
                        heartbeat_client.Send(bytes, bytes.Length, server_endpoint);
                    } else
                    {
                        Debug.Log("Rand Err : No such random number in rand_2_start_and_stop nor rand_2_hash");
                    }

                }

                break;
        }
    }

    private void HandleRpcMessage(ByteString data)
    {
        ADS.RpcRequest rpc_request = ADS.RpcRequest.Parser.ParseFrom(data); 
        // Debug.Log("RPC " + rpc_request.Rpc + " received");
        HandleRPC(rpc_request.Rpc);
    }

    private void HandleStatistics(ByteString data)
    {
        ADS.StatisticsResponse stats_response = ADS.StatisticsResponse.Parser.ParseFrom(data);
        UInt32 hash = stats_response.Hash;
        UInt64 recv_us = stats_response.RecvUs;
        UInt64 handle_us = stats_response.HandleUs;
        string data_center = stats_response.Datacenter;
        if (hash != last_rand_hash) {
            Debug.Log("HandleStatistics Err: not last rand hash");
            return;
        }

        if (data_center_2_deltas.ContainsKey(data_center))
            data_center_2_deltas[data_center] = (recv_us, handle_us);
        else
        {
            data_center_2_deltas.Add(data_center, (recv_us, handle_us));
        }

        log_statistics();
    }

    private void HandlePresenceMessage(ByteString data)
    {
        ADS.PresenceResponse presence_response = ADS.PresenceResponse.Parser.ParseFrom(data);
        if(!config.lobby.Equals(presence_response.LobbyName))
        {
            Debug.Log("HandlePresenceMessage Err: This lobby " + presence_response.LobbyName + " is different from our joined lobby's name");
            return;
        }

        Dictionary<int, string> new_presence = new Dictionary<int, string>();
        for (int i = 0; i < presence_response.PlayerInfos.Count; i++)
        {
            int player_id = presence_response.PlayerInfos[i].PlayerId;
            string player_name = presence_response.PlayerInfos[i].PlayerName;
            string data_center = presence_response.PlayerInfos[i].Datacenter;
            new_presence.Add(player_id, player_name);
            if (player_2_infos.ContainsKey(player_id)){
                player_2_infos[player_id] = (player_name, data_center);
            } else
            {
                player_2_infos.Add(player_id, (player_name, data_center));
            }
        }

        update_players_presence(new_presence);
    }


    private void update_players_presence(Dictionary<int, string> new_presence)
    {
        foreach (KeyValuePair<int, Avatar> kv_avatar in view.avatars)
        {
            if (!new_presence.ContainsKey(kv_avatar.Key))
            {
                kv_avatar.Value.to_be_destroyed = true;
                // Debug.Log("Marked " + kv_avatar.Key + " for destruction");
            }
        }
        players_presence = new Dictionary<int, string>(new_presence);
    }

    private uint hash(byte[] data) {
        uint hash = 5381;
        for (int i = 0; i < data.Length; i++)
            hash = ((hash << 5) + hash) + data[i];  /* hash * 33 + str[i] */
        return hash;
    }


    private bool is_synced()
    {
        return !ingest_server_id.Equals(DEFAULT_SERVER_ID);
    }

    private void ReceiveData()
    {
        while (true)
        {   
            try 
            {
                IPEndPoint from_endpoint = new IPEndPoint(IPAddress.Any, 0);

                // unserialization
                byte[] received_data_bytes = receive_client.Receive(ref from_endpoint);
                ADS.Message response = ADS.Message.Parser.ParseFrom(received_data_bytes);
                //last_packet = "[ type=" + response.Type.ToString() + " sender_id=" + response.SenderId + " digest=" + hash(received_data_bytes) + " ]";
                //Debug.Log(last_packet);

                switch(response.Type)
                {
                    case ADS.Type.World:
                        HandleWorldMessage(response.Data);
                        break;
                    case ADS.Type.Heartbeat:
                        HandleHeartbeatMessage(response.Data);
                        break;
                    case ADS.Type.Syn:
                        HandleSynMessage(response.Data);
                        break;
                    case ADS.Type.Presence:
                        HandlePresenceMessage(response.Data);
                        break;
                    case ADS.Type.Rpc:
                        HandleRpcMessage(response.Data);
                        break;
                    case ADS.Type.Statistics:
                        HandleStatistics(response.Data);
                        break;
                    default:
                        Debug.Log("ReceiveData Err : Unprocessed message type");
                        break;
                }
            } 
            catch (Exception e)
            {
               //Debug.Log("Exception in ReceiveData(): " + e);
            }
        }
    }

    private void Heartbeat() {
        while (true) {
            if (!is_synced())
                continue;

            System.Threading.Thread.Sleep(config.heartbeat_sleep_ms);
            try 
            {
                ping_start_time = DateTime.Now;

                byte[] data = GetMessageByteArray(ADS.Type.Heartbeat);
                heartbeat_client.Send(data, data.Length, server_endpoint);

                int random_num = rand.Next(MAX_RANDOM_NUM);
                rand_2_start_unix_time_millisec.Add(random_num, ((DateTimeOffset)DateTime.Now).ToUnixTimeMilliseconds());
                byte[] bytes = GetMessageByteArray(ADS.Type.Rpc,
                    (
                        new ADS.RpcRequest
                        {
                            Rpc = RPC_RAND_STR_PREFIX + random_num.ToString(random_str_format),
                            InitiatorId = config.player_id
                        }
                    ).ToByteString()
                );
                heartbeat_client.Send(bytes, bytes.Length, server_endpoint);
                rand_2_hash.Add(random_num, hash(bytes));
                last_rand_hash = rand_2_hash[random_num];
            }
            catch (Exception e)
            {
                //Debug.Log("Exception in Heartbeat(): " + e);
            }
        }
    }

    private void SendContinuousData()
    {
        while (true)
        {
            if (!is_synced())
                continue;

            System.Threading.Thread.Sleep(config.controller_sleep_ms);
            try {
                // serialization
                Vector3 head_pos = device_watcher.GetHeadPosition();
                Quaternion head_rot = device_watcher.GetHeadRotation();
                Vector3 right_controller_pos = device_watcher.GetRightControllerPosition();
                Quaternion right_controller_rot = device_watcher.GetRightControllerRotation();
                Vector3 left_controller_pos = device_watcher.GetLeftControllerPosition();
                Quaternion left_controller_rot = device_watcher.GetLeftControllerRotation();
                Vector2 right_joystick = device_watcher.GetRightJoystickVec2();
                Vector2 left_joystick = device_watcher.GetLeftJoystickVec2();
                Vector3 forward_direction = ComputeDesiredMove(left_joystick);
                ADS.ContinuousRequest continuous_request = new ADS.ContinuousRequest
                {
                    Data = new ADS.ContinuousData
                    {
                        Headset = new ADS.Transform
                        {
                            Position = new ADS.Vector3
                            {
                                X = head_pos.x,
                                Y = head_pos.y,
                                Z = head_pos.z
                            },
                            Rotation = new ADS.Quaternion
                            {
                                X = head_rot.x,
                                Y = head_rot.y,
                                Z = head_rot.z,
                                W = head_rot.w
                            }
                        },
                        RightController = new ADS.Transform
                        {
                            Position = new ADS.Vector3
                            {
                                X = right_controller_pos.x,
                                Y = right_controller_pos.y,
                                Z = right_controller_pos.z
                            },
                            Rotation = new ADS.Quaternion
                            {
                                X = right_controller_rot.x,
                                Y = right_controller_rot.y,
                                Z = right_controller_rot.z,
                                W = right_controller_rot.w
                            }
                        },
                        LeftController = new ADS.Transform
                        {
                            Position = new ADS.Vector3
                            {
                                X = left_controller_pos.x,
                                Y = left_controller_pos.y,
                                Z = left_controller_pos.z
                            },
                            Rotation = new ADS.Quaternion
                            {
                                X = left_controller_rot.x,
                                Y = left_controller_rot.y,
                                Z = left_controller_rot.z,
                                W = left_controller_rot.w
                            }
                        },
                        RightJoystick = new ADS.Vector2
                        {
                            X = right_joystick.x,
                            Y = right_joystick.y
                        },
                        LeftJoystick = new ADS.Vector2
                        {
                            X = forward_direction.x,
                            Y = forward_direction.z
                        },
                        PlayerOffset = new ADS.Vector3
                        {
                            X = 0f,
                            Y = 0f,
                            Z = 0f
                        }
                    }
                };

                byte[] bytes = GetMessageByteArray(ADS.Type.Continuous, continuous_request.ToByteString());
                continuous_input_client.Send(bytes, bytes.Length, server_endpoint);
            }
            catch (Exception e)
            {
                //Debug.Log("Exception in SendControllerData(): " + e);
            }
        }
    }

    private void SendDiscreteData()
    {
        while (true)
        {
            System.Threading.Thread.Sleep(discrete_timeout_ms);
            try {
                bool is_pushed = device_watcher.GetLeftControllerPrimaryButtonPushed();
                if (current_discrete_state[OculusButtons.LEFT_PRIMARY_BUTTON] != is_pushed)
                    HandleLeftPrimaryButtonEvent(is_pushed);
                
                is_pushed = device_watcher.GetRightControllerPrimaryButtonPushed();
                if (current_discrete_state[OculusButtons.RIGHT_PRIMARY_BUTTON] != is_pushed)
                    HandleRightPrimaryButtonEvent(is_pushed);

                is_pushed = device_watcher.GetLeftControllerSecondaryButtonPushed();
                if (current_discrete_state[OculusButtons.LEFT_SECONDARY_BUTTON] != is_pushed)
                    HandleLeftSecondaryButtonEvent(is_pushed);
                
                is_pushed = device_watcher.GetRightControllerSecondaryButtonPushed();
                if (current_discrete_state[OculusButtons.RIGHT_SECODNARY_BUTTON] != is_pushed)
                    HandleRightSecondaryButtonEvent(is_pushed);

                is_pushed = device_watcher.GetRightControllerTriggerButtonPushed();
                if (current_discrete_state[OculusButtons.RIGHT_TRIGGER_BUTTON] != is_pushed)
                    HandleRightTriggerButtonEvent(is_pushed);

            }
            catch (Exception e)
            {
                //Debug.Log("Exception in SendControllerData(): " + e);
            }
        }
    }

    private void HandleLeftPrimaryButtonEvent(bool is_pushed) {
        //serialization
        ADS.RpcRequest rpc_request;
        current_discrete_state[OculusButtons.LEFT_PRIMARY_BUTTON] = is_pushed;
        if (is_pushed) {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.LEFT_PRIMARY_BUTTON_DOWN,
                InitiatorId = config.player_id
            };
        }
        else
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.LEFT_PRIMARY_BUTTON_UP,
                InitiatorId = config.player_id
            };

        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc, rpc_request.ToByteString());
        discrete_input_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void HandleRightPrimaryButtonEvent(bool is_pushed) {
        ADS.RpcRequest rpc_request;
        current_discrete_state[OculusButtons.RIGHT_PRIMARY_BUTTON] = is_pushed;
        if (is_pushed) // if button pushed
        {
            device_watcher.RumbleRightController();// local rumble
            rpc_request = new ADS.RpcRequest
            {
                InitiatorId = config.player_id,
                Rpc = RPCs.HAPTICS
            };

            //Debug.Log("Vibration: Sending RPC Haptics to server");
        }
        else
        {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.RIGHT_SECONDARY_BUTTON_UP,
                InitiatorId = config.player_id
            };
        }

        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc, rpc_request.ToByteString());
        discrete_input_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void HandleLeftSecondaryButtonEvent(bool is_pushed) {
        ADS.RpcRequest rpc_request;
        current_discrete_state[OculusButtons.LEFT_SECONDARY_BUTTON] = is_pushed;
        if (is_pushed) // if button pushed
        {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.LEFT_SECONDARY_BUTTON_DOWN,
                InitiatorId = config.player_id
            };
        }
        else
        {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.LEFT_SECONDARY_BUTTON_UP,
                InitiatorId = config.player_id
            };
        }

        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc, rpc_request.ToByteString());
        discrete_input_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void HandleRightSecondaryButtonEvent(bool is_pushed) {
        ADS.RpcRequest rpc_request;
        current_discrete_state[OculusButtons.RIGHT_SECODNARY_BUTTON] = is_pushed;
        if (is_pushed) // if button pushed
        {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.RIGHT_SECONDARY_BUTTON_DOWN,
                InitiatorId = config.player_id
            };
        }
        else
        {
            rpc_request = new ADS.RpcRequest
            {
                Rpc = RPCs.RIGHT_SECONDARY_BUTTON_UP,
                InitiatorId = config.player_id
            };
        }

        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc, rpc_request.ToByteString());
        discrete_input_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void FixedUpdate()
    {
        RaycastHit hit;

        Quaternion device_rotation = device_watcher.GetRightControllerRotation();
        Vector3 ray_start_position = device_watcher.GetRightControllerPosition() + view.POS_OFFSET;
        Vector3 ray_direction = device_rotation * Vector3.forward; // axis
        float ray_length = 10.0f; //same as XR raycaster

        frame_count += 1;
        if(frame_count == color_fade_frames)
        {
            frame_count = 0;
            // set confirm button white
            join_btn_object.GetComponent<Renderer>().material.SetColor("_Color", Color.white);
            //leave_btn_object.GetComponent<Renderer>().material.SetColor("_Color", Color.white); 
            debug_btn_object.GetComponent<Renderer>().material.SetColor("_Color", Color.white);
            minimap_btn_object.GetComponent<Renderer>().material.SetColor("_Color", Color.white);
        }

        if (Physics.Raycast(ray_start_position, ray_direction, out hit, ray_length))
        {
            action_object = hit.transform.gameObject;
        }
    }

    private void handle_ingest_server_trigger()
    {
        if (!object_2_ingest_server.ContainsKey(action_object))
            return;

        if (is_synced())
        {
            // Debug.Log("Confirm Error: already synced");
            return;
        }

        ingest_server_ip = object_2_ingest_server[action_object].Item1;
        ingest_server_name = object_2_ingest_server[action_object].Item2;
        // Debug.Log("set server:" + object_2_ingest_server[action_object]);
        foreach (KeyValuePair<GameObject, (String,String)> object_str in object_2_ingest_server) // change color
        {
            object_str.Key.GetComponent<Renderer>().material.SetColor("_Color", Color.white);
        }
        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);
        is_server_set = true;
    }

    private void handle_username_trigger()
    {
        if (!object_2_username.ContainsKey(action_object))
            return;

        if (is_synced())
        {
            // Debug.Log("Confirm Error: already synced");
            return;
        }

        config.player_name = object_2_username[action_object];
        // Debug.Log("set username:" + object_2_username[action_object]);
        foreach (KeyValuePair<GameObject, String> object_str in object_2_username) // change color
        {
            object_str.Key.GetComponent<Renderer>().material.SetColor("_Color", Color.white);
        }
        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);
        is_username_set = true;
    }

    private void handle_join_trigger()
    {
        if (action_object != join_btn_object)
            return;
        if (!is_server_set || !is_username_set)
        {
            // Debug.Log("Confirm Error: servername or username is not set");
            return;
        }
        if (is_synced())
        {
            // Debug.Log("Confirm Error: already synced");
            return;
        }

        ADS.SynRequest syn_request = new ADS.SynRequest
        {
            PlayerName = config.player_name,
            LobbyName = config.lobby,
            IngestServerId = 0,
            ServerName = ingest_server_name
        };
        //Debug.Log("HandleRightTriggerButtonEvent : sending syn request");
        byte[] temp = GetMessageByteArray(ADS.Type.Syn, syn_request.ToByteString());
        server_endpoint = new IPEndPoint(IPAddress.Parse(ingest_server_ip), ingest_server_port);
        receive_client.Send(temp, temp.Length, server_endpoint);

        // Debug.Log("SynRequest" + DebugDisplay.SPLITTER + "sent to " + ingest_server_ip + "(" + ingest_server_port + ")");
        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);
    }

//    private void handle_leave_trigger()
//    {
//        if (action_object != leave_btn_object)
//            return;

//        if (!is_synced())
//        {
//            // Debug.Log("Confirm Error: not synced");
//            return;
//        }

//        byte[] temp = GetMessageByteArray(ADS.Type.Fin);

//        server_endpoint = new IPEndPoint(IPAddress.Parse(ingest_server_ip), ingest_server_port);
//        receive_client.Send(temp, temp.Length, server_endpoint);

//        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);

//        ingest_server_id = DEFAULT_SERVER_ID;
//        //TODO: leave recenter avatar
//        view.reset_xr_rig_offset();

//        //TODO: clean avatar obejcts.
//        view.destroy_all_avatar_objects();
//}

    private void handle_minimap_trigger()
    {
        if (action_object != minimap_btn_object)
            return;

        view.is_show_birdview = !view.minimap_canvas.activeSelf;
        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);
    }

    private void handle_debug_trigger()
    {
        if (action_object != debug_btn_object)
            return;

        // Change state of debug canvas on left primary push (X Button)
        view.is_show_debug = !view.debug_canvas.activeSelf;
        action_object.GetComponent<Renderer>().material.SetColor("_Color", Color.red);
    }

    private void handle_interactable_trigger()
    {
        if (!view.is_interactable(action_object))
            return;

        string triggered_item_name = view.get_interactable_name(action_object);
        if (triggered_item_name == "")
            return;

        //serialization
        ADS.RpcRequest rpc_request = new ADS.RpcRequest
        {
            Rpc = (item_claimed_by_current_user.Contains(triggered_item_name) ? RPC_UNCLAIM_STR_PREFIX: RPC_CLAIM_STR_PREFIX) + triggered_item_name,
            InitiatorId = config.player_id
        };


        byte[] bytes = GetMessageByteArray(ADS.Type.Rpc, rpc_request.ToByteString());
        discrete_input_client.Send(bytes, bytes.Length, server_endpoint);
    }

    private void HandleRightTriggerButtonEvent(bool is_pushed)
    {
        //Debug.Log("Right controller trigger pushed\n");
        current_discrete_state[OculusButtons.RIGHT_TRIGGER_BUTTON] = is_pushed;

        if (!is_pushed)
            return;

        handle_ingest_server_trigger();
        handle_username_trigger();
        handle_join_trigger();
        //handle_leave_trigger(); 
        handle_minimap_trigger();
        handle_debug_trigger();
        handle_interactable_trigger(); 
    }

    private Byte[] GetMessageByteArray(ADS.Type type, ByteString data = null)
    {
        data = (data == null) ? ByteString.Empty : data;
        return new ADS.Message
        {
            SenderId = config.player_id,
            Type = type,
            Data = data
        }.ToByteArray();
    }

    protected virtual Vector3 ComputeDesiredMove(Vector2 input)
    {
        if (input == Vector2.zero)
            return Vector3.zero;

        var xrOrigin = view.xr_origin;
        if (xrOrigin == null)
            return Vector3.zero;

        // Assumes that the input axes are in the range [-1, 1].
        // Clamps the magnitude of the input direction to prevent faster speed when moving diagonally,
        // while still allowing for analog input to move slower (which would be lost if simply normalizing).
        //var m_EnableStrafe = true;
        //var inputMove = Vector3.ClampMagnitude(new Vector3(m_EnableStrafe ? input.x : 0f, 0f, input.y), 1f);
        var inputMove = Vector3.ClampMagnitude(new Vector3(input.x, 0f, input.y), 1f);

        var originTransform = xrOrigin.Origin.transform;
        var originUp = originTransform.up;

        // Determine frame of reference for what the input direction is relative to
        // var forwardSourceTransform = m_ForwardSource == null ? xrOrigin.Camera.transform : m_ForwardSource;
        // var forwardSourceTransform = m_ForwardSource == null ? xrOrigin.Camera.transform : m_ForwardSource;
        // var inputForwardInWorldSpace = forwardSourceTransform.forward;
        var forwardSourceTransform = xrOrigin.Camera.transform;
        var inputForwardInWorldSpace = forwardSourceTransform.forward;
        if (Mathf.Approximately(Mathf.Abs(Vector3.Dot(inputForwardInWorldSpace, originUp)), 1f))
        {
            // When the input forward direction is parallel with the rig normal,
            // it will probably feel better for the player to move along the same direction
            // as if they tilted forward or up some rather than moving in the rig forward direction.
            // It also will probably be a better experience to at least move in a direction
            // rather than stopping if the head/controller is oriented such that it is perpendicular with the rig.
            inputForwardInWorldSpace = -forwardSourceTransform.up;
        }

        var inputForwardProjectedInWorldSpace = Vector3.ProjectOnPlane(inputForwardInWorldSpace, originUp);
        var forwardRotation = Quaternion.FromToRotation(originTransform.forward, inputForwardProjectedInWorldSpace);

        var translationInRigSpace = forwardRotation * inputMove; // * (m_MoveSpeed * Time.deltaTime);
        var translationInWorldSpace = originTransform.TransformDirection(translationInRigSpace);

        return translationInWorldSpace;
    }
}
