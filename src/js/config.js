module.exports = [
  { "type": "heading", "defaultValue": "Example Config Page" },
  {
    type:"section",
    items:
    [
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
        defaultValue:"+18"
      }
    ]
  },
  {
    type:"submit",
    defaultValue:"save"
  }
];