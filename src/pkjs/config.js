module.exports = [
  { "type": "heading", "defaultValue": "Time Slider Config Page" },
  {
    type:"section",
    items:
    [
      {
        type: "heading",
        defaultValue: "UI Settings"
      },
      {
        type:"toggle", 
        "messageKey": "CONFIG_METRIC",
        "label": "Metric",
        defaultValue:true
      },
      {
        type:"toggle", 
        "messageKey": "CONFIG_BUZZ",
        "label": "Vibrate on Bt disconnect",
        defaultValue:true
      },
      {
        type:"toggle", 
        messageKey: "CONFIG_BUZZ_MUTE",
        label: "Mute vibration at night",
        defaultValue:true
      }
    ]
  },
  {
    type:"heading",
    defaultValue:"TZ Offset (0 to disable)"
  },
  {
    type:"section",
    items:
    [
      {
        type:"input",
        messageKey:"CONFIG_TZ_OFFSET",
        defaultValue:"18"
      }
    ]
  },
  {
    type:"submit",
    defaultValue:"save"
  }
];