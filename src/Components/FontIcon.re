/*
 * FontIcon.re
 *
 * Helper component for using icon fonts, like FontAwesome
 */

open Revery.UI;
module ZedBundled = Oni_Core.ZedBundled;

module Styles = {
  open Style;

  let text = (~color) => [
    Style.color(color),
    textWrap(Revery.TextWrapping.NoWrap),
  ];
};

let codeToIcon = icon => ZedBundled.singleton(Uchar.of_int(icon));

let make =
    (
      ~icon,
      ~fontFamily=FontAwesome.FontFamily.solid,
      ~fontSize=15.,
      ~color,
      (),
    ) =>
  <Text
    text={codeToIcon(icon)}
    style={Styles.text(~color)}
    fontSize
    fontFamily
  />;
