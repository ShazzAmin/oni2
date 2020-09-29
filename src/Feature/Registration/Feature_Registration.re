open Oni_Core;

type model = {licenseKey: option(string)};

let initial = {licenseKey: None};

[@deriving show({with_path: false})]
type command =
  | EnterLicenseKey;

[@deriving show({with_path: false})]
type msg =
  | Command(command);

type outmsg =
  | Nothing
  | ShowLicenseKeyInput
  | Effect(Isolinear.Effect.t(msg));

module Commands = {
  open Feature_Commands.Schema;

  let enterLicenseKey =
    define(
      ~category="Oni2",
      ~title="Enter license key",
      "oni.app.enterLicenseKey",
      Command(EnterLicenseKey),
    );
};

module Contributions = {
  let commands = Commands.[enterLicenseKey];
};

let update = (model, msg) =>
  switch (msg) {
  | Command(EnterLicenseKey) => (model, ShowLicenseKeyInput)
  };
