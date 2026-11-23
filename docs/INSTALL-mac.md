JG MASTERING EQ - HOW TO INSTALL ON A MAC (prototype builds)
=============================================================

What's in the download
- JG Mastering EQ.component  -> the AU version (Logic, GarageBand, most Mac DAWs)
- JG Mastering EQ.vst3       -> the VST3 version (Ableton Live, Reaper, Cubase, Studio One...)
- JG Mastering EQ.app        -> a standalone app, handy for a quick look without a DAW

1. Copy the plugins into place
   In Finder choose Go > Go to Folder... (Shift-Cmd-G) and paste:
       ~/Library/Audio/Plug-Ins/Components
   Drag "JG Mastering EQ.component" into that folder.
   Then go to:
       ~/Library/Audio/Plug-Ins/VST3
   and drag "JG Mastering EQ.vst3" into that folder.
   (If a folder doesn't exist yet, create it with that exact name.)

2. Let macOS open them (one time only)
   These prototype builds are not yet signed with an Apple Developer ID, so
   macOS blocks files downloaded from the internet. Open Terminal
   (Applications > Utilities > Terminal), paste this whole line and press Return:

       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"JG Mastering EQ.component" ~/Library/Audio/Plug-Ins/VST3/"JG Mastering EQ.vst3"

   It prints nothing when it works. For the standalone app, right-click it and
   choose Open, then Open again.

3. Rescan in your DAW
   - Logic: quit and reopen Logic. If it doesn't appear, open
     Logic Pro > Settings > Plug-in Manager, find "JG Designs", and click
     "Reset & Rescan Selection". It lives under Audio Units > JG Designs.
   - Ableton Live: Settings > Plug-Ins > make sure "Use VST3 Plug-In Custom
     Folder" or system folders is on, then click Rescan.
   - Reaper: Settings > Plug-ins > VST > Re-scan.

Good to know about prototype builds
- Audio passes through untouched. The knobs move and are saved/automatable,
  but they don't change the sound yet.
- Settings saved in a session with a prototype may not come back in later
  builds, because the control list is still being finalised.
- Updating: delete the old .component/.vst3 from the folders above, copy the
  new ones in, run step 2 again, and restart the DAW.
