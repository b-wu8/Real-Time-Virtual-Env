/**
 *  Displays an interface for the user to change names and/or lobbies. 
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class SettingsDisplay : MonoBehaviour
{
    private TouchScreenKeyboard keyboard;
    public static string input = "";

    public void getFields()
    {
        keyboard = TouchScreenKeyboard.Open("", TouchScreenKeyboardType.Default);
        if (keyboard != null) 
        {
            input = keyboard.text;
        }
    }
}