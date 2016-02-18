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
        "appKey": "CONFIG_METRIC",
        "label": "Metric",
        defaultValue:true
      },
      {
        type:"toggle", 
        "appKey": "CONFIG_BUZZ",
        "label": "Vibrate on Bt disconnect",
        defaultValue:true
      },
      {
        type:"toggle", 
        appKey: "CONFIG_BUZZ_MUTE",
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
        appKey:"CONFIG_TZ_OFFSET",
        defaultValue:"18",
        attributes: {
          type: "number",
          min: "-24",
          max: "24"
        }
      }
    ]
  },
  {
    type:"submit",
    defaultValue:"save"
  }
];