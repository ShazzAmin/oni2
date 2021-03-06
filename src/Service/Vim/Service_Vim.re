open EditorCoreTypes;
open Oni_Core;
module Log = (val Log.withNamespace("Service_Vim"));

let forceReload = () =>
  Isolinear.Effect.create(~name="vim.discardChanges", () =>
    ignore(Vim.command("e!"): Vim.Context.t)
  );

let forceOverwrite = () =>
  Isolinear.Effect.create(~name="vim.forceOverwrite", () =>
    ignore(Vim.command("w!"): Vim.Context.t)
  );

let reload = () =>
  Isolinear.Effect.create(~name="vim.reload", () => {
    ignore(Vim.command("e"): Vim.Context.t)
  });

let saveAllAndQuit = () =>
  Isolinear.Effect.create(~name="lifecycle.saveAllAndQuit", () =>
    ignore(Vim.command("xa"): Vim.Context.t)
  );

let quitAll = () =>
  Isolinear.Effect.create(~name="lifecycle.saveAllAndQuit", () =>
    ignore(Vim.command("qa!"): Vim.Context.t)
  );

module Effects = {
  let paste = (~toMsg, text) => {
    Isolinear.Effect.createWithDispatch(~name="vim.clipboardPaste", dispatch => {
      let isCmdLineMode = Vim.Mode.current() == Vim.Mode.CommandLine;
      let isInsertMode = Vim.Mode.current() == Vim.Mode.Insert;

      if (isInsertMode || isCmdLineMode) {
        if (!isCmdLineMode) {
          Vim.command("set paste") |> ignore;
        };

        Log.infof(m => m("Pasting: %s", text));
        let latestContext: Vim.Context.t = Oni_Core.VimEx.inputString(text);

        if (!isCmdLineMode) {
          Vim.command("set nopaste") |> ignore;
          dispatch(toMsg(latestContext.cursors));
        };
      };
    });
  };

  let getRegisterValue = (~toMsg, char) =>
    Isolinear.Effect.createWithDispatch(~name="vim.getRegisterValue", dispatch => {
      let result = Vim.Registers.get(~register=char);
      dispatch(toMsg(result));
    });

  let applyEdits =
      (
        ~bufferId: int,
        ~version: int,
        ~edits: list(Vim.Edit.t),
        toMsg: result(unit, string) => 'msg,
      ) => {
    Isolinear.Effect.createWithDispatch(~name="vim.applyEdits", dispatch => {
      Log.infof(m => m("Trying to apply edits to buffer: %d", bufferId));
      let maybeBuffer = Vim.Buffer.getById(bufferId);
      let result =
        switch (maybeBuffer) {
        | None =>
          Error("No buffer found with id: " ++ string_of_int(bufferId))
        | Some(buffer) =>
          let bufferVersion = Vim.Buffer.getVersion(buffer);
          if (bufferVersion < version) {
            Error(
              Printf.sprintf(
                "Expected buffer version %d, got %d",
                bufferVersion,
                version,
              ),
            );
          } else {
            Vim.Buffer.applyEdits(~edits, buffer);
          };
        };

      result |> toMsg |> dispatch;
    });
  };
  let applyCompletion = (~meetColumn, ~insertText, ~toMsg) =>
    Isolinear.Effect.createWithDispatch(~name="applyCompletion", dispatch => {
      let cursor = Vim.Cursor.get();
      // TODO: Does this logic correctly handle unicode characters?
      let delta =
        ByteIndex.toInt(cursor.byte) - CharacterIndex.toInt(meetColumn);

      let _: Vim.Context.t = VimEx.repeatKey(delta, "<BS>");
      let {cursors, _}: Vim.Context.t = VimEx.inputString(insertText);

      dispatch(toMsg(cursors));
    });

  let loadBuffer = (~filePath: string, toMsg) => {
    Isolinear.Effect.createWithDispatch(~name="loadBuffer", dispatch => {
      let currentBuffer = Vim.Buffer.getCurrent();

      let newBuffer = Vim.Buffer.openFile(filePath);

      // Revert to previous buffer
      Vim.Buffer.setCurrent(currentBuffer);

      dispatch(toMsg(~bufferId=Vim.Buffer.getId(newBuffer)));
    });
  };
};

module Sub = {
  module EvalSubscription =
    Isolinear.Sub.Make({
      type state = unit;
      type params = string;
      type msg = result(string, string);
      let name = "Vim.Sub.Eval";
      let id = params => params;
      let init = (~params, ~dispatch) => {
        Vim.eval(params) |> dispatch;
      };
      let update = (~params as _, ~state as _, ~dispatch as _) => {
        ();
      };
      let dispose = (~params as _, ~state as _) => {
        ();
      };
    });
  let eval = (~toMsg, expression) => {
    EvalSubscription.create(expression) |> Isolinear.Sub.map(toMsg);
  };
};
