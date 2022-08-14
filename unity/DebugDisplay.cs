/**
 *  This is a script for printing debug log onto an Unity UI text. 
 */

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

//public class DebugDisplay : MonoBehaviour
//{
//    public static string SPLITTER = "::";
//    Dictionary<string, string> debug_logs = new Dictionary<string, string>();
//    public string display_text = "";
//    public string stack = "";
//    public bool is_active = false;
//    public GameObject debug_canvas, debug_panel, debug_text;

//    public void Start()
//    {
//        // debug canvas
//        debug_canvas = new GameObject("Debug Info Canvas");
//        debug_canvas.AddComponent<Canvas>();
//        debug_canvas.GetComponent<Canvas>().renderMode = RenderMode.WorldSpace;
//        debug_canvas.AddComponent<CanvasScaler>();
//        debug_canvas.GetComponent<CanvasScaler>().dynamicPixelsPerUnit = 120;
//        debug_canvas.AddComponent<GraphicRaycaster>();
//        // debug_canvas.AddComponent<DebugDisplay>();

//        RectTransform debug_canvas_rectTransform = debug_canvas.GetComponent<RectTransform>();
//        debug_canvas_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
//        debug_canvas_rectTransform.sizeDelta = new Vector2(200f, 100f);
//        debug_canvas_rectTransform.anchorMin = new Vector2(0f, 0f);
//        debug_canvas_rectTransform.anchorMax = new Vector2(1f, 1f);
//        debug_canvas_rectTransform.pivot = new Vector2(0.5f, 0.5f);
//        debug_canvas_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
//        debug_canvas_rectTransform.localScale = new Vector3(0.05f, 0.05f, 0.05f);

//        // debug panel
//        debug_panel = new GameObject("Debug Info Panel");
//        debug_panel.transform.SetParent(debug_canvas.transform);
//        debug_panel.AddComponent<CanvasRenderer>();
//        Image i_debug = debug_panel.AddComponent<Image>();
//        i_debug.color = Color.black;

//        // Provide Panel position and size using RectTransform
//        RectTransform debug_panel_rectTransform = debug_panel.GetComponent<RectTransform>();
//        debug_panel_rectTransform.anchorMin = new Vector2(0f, 0f);
//        debug_panel_rectTransform.anchorMax = new Vector2(1f, 1f);
//        debug_panel_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
//        debug_panel_rectTransform.localScale = new Vector3(1f, 1f, 1f);
//        debug_panel_rectTransform.pivot = new Vector2(0.5f, 0.5f);
//        debug_panel_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
//        debug_panel_rectTransform.offsetMin = new Vector2(0f, 0f);
//        debug_panel_rectTransform.offsetMax = new Vector2(0f, 0f);

//        // Load the Arial font from the Unity Resources folder
//        Font arial;
//        arial = (Font)Resources.GetBuiltinResource(typeof(Font), "Arial.ttf");

//        // debug text
//        debug_text = new GameObject("Debug Info Text");
//        debug_text.transform.SetParent(debug_panel.transform);
//        debug_text.AddComponent<Text>();

//        Text debug_texts = debug_text.GetComponent<Text>();
//        debug_texts.font = arial;
//        debug_texts.text = "Debug display";
//        debug_texts.fontSize = 5;
//        debug_texts.color = Color.green;
//        debug_texts.horizontalOverflow = HorizontalWrapMode.Overflow;
//        debug_texts.verticalOverflow = VerticalWrapMode.Overflow;

//        // Provide debug Text position and size using RectTransform
//        RectTransform debug_text_rectTransform;
//        debug_text_rectTransform = debug_text.GetComponent<RectTransform>();
//        debug_text_rectTransform.anchorMin = new Vector2(0f, 0f);
//        debug_text_rectTransform.anchorMax = new Vector2(1f, 1f);
//        debug_text_rectTransform.localPosition = new Vector3(5f, 2.5f, 0f);
//        debug_text_rectTransform.pivot = new Vector2(0.5f, 0.5f);
//        debug_text_rectTransform.rotation = Quaternion.Euler(0f, 90f, 0f);
//        debug_text_rectTransform.localScale = new Vector3(1f, 1f, 1f);
//        debug_text_rectTransform.offsetMin = new Vector2(10f, 10f);
//        debug_text_rectTransform.offsetMax = new Vector2(-10f, -10f);

//        // debug_canvas.GetComponent<DebugDisplay>().display = debug_texts;
//        debug_canvas.SetActive(is_active);
//    }

//    void OnEnable()
//    {
//        Application.logMessageReceivedThreaded += HandleLog;
//        // display.fontSize = 5;
//    }
//    void OnDisable()
//    {
//        Application.logMessageReceivedThreaded -= HandleLog;
//    }

//    public void Update()
//    {
//        if (is_active != debug_canvas.activeSelf)
//            debug_canvas.SetActive(is_active);
//        if (debug_canvas.activeSelf)
//            debug_text.GetComponent<Text>().text = display_text;
//    }

//    void HandleLog(string log_string, string stack_trace, LogType type)
//    {
//        if (type != LogType.Log)
//        {
//            string[] split_string = log_string.Split(new string[] { SPLITTER }, StringSplitOptions.None);
//            if (split_string.Length > 1)
//            {
//                string debug_key = split_string[0];
//                string debug_value = split_string[1];
//                if (debug_logs.ContainsKey(debug_key))
//                    debug_logs[debug_key] = debug_value;
//                else
//                    debug_logs.Add(debug_key, debug_value);
//            }
//        }

//        if (type == LogType.Exception)
//        {
//            string debug_key = "Exception";
//            if (debug_logs.ContainsKey(debug_key))
//                debug_logs[debug_key] = log_string;
//            else
//                debug_logs.Add(debug_key, log_string);
//        }

//        display_text = "Debug Logs:\n";
//        Dictionary<string, string> debug_log_copy = new Dictionary<string, string>(debug_logs);  // Because we're not in main thread
//        int num_logs_left = 10;
//        foreach (KeyValuePair<string, string> log in debug_log_copy)
//        {
//            if (log.Value != "")
//                display_text += log.Key + " " + SPLITTER + " " + log.Value + "\n";
//            if (--num_logs_left < 0)
//                break;
//        }
//    }
//}


public class DebugDisplay : MonoBehaviour
{
    Dictionary<string, string> debug_logs = new Dictionary<string, string>();
    string display_text = "";
    public string stack = "";
    public Text display;
    public static string SPLITTER = "::";
    private int MAX_LINES_OF_LOG = 100;

    void OnEnable()
    {
        Application.logMessageReceivedThreaded += HandleLog;
    }
    void OnDisable()
    {
        Application.logMessageReceivedThreaded -= HandleLog;
    }

    public void Update()
    {
        display.text = display_text;
    }

    void HandleLog(string logString, string stack_trace, LogType type)
    {
        if (type == LogType.Log)
        {
            string[] splitString = logString.Split(char.Parse(":"));
            string debug_key = splitString[0];
            string debug_value = splitString.Length > 1 ? splitString[1] : "";

            if (debug_logs.ContainsKey(debug_key))
                debug_logs[debug_key] = debug_value;
            else
                debug_logs.Add(debug_key, debug_value);
        }

        if (type == LogType.Exception)
        {
            string[] splitString = logString.Split(char.Parse(":"));
            string debug_key = splitString[0];
            string debug_value = splitString.Length > 1 ? splitString[1] : "";

            if (debug_logs.ContainsKey(debug_key))
                debug_logs[debug_key] = debug_value;
            else
                debug_logs.Add(debug_key, debug_value);


            if (debug_logs.ContainsKey("Stack"))
                debug_logs[debug_key] = stack_trace;
            else
                debug_logs.Add("Stack", stack_trace);
        }

        display_text = "";
        Dictionary<string, string> debug_log_copy = new Dictionary<string, string>(debug_logs);
        int line_counter = 0;
        foreach (KeyValuePair<string, string> log in debug_log_copy)
        {
            if (log.Value == "")
                display_text += log.Key + "\n";
            else
                display_text += log.Key + ": " + log.Value + "\n";
            line_counter += 1;
            if (line_counter == MAX_LINES_OF_LOG)
                return;
        }
    }
}

