# PermToFBX
A simple CLI tool that exports models from perm.bin to FBX format.

The tool automatically parses all loaded files and exports only the models. 

If you want textures to be exported as well, you must load the texture specific files alongside the "model" files.

## Basic CLI Usage

`PermToFBX.exe -rig=BasicFemale "CharacterRigs.bin" "Sandra.perm.bin" "Sandra_TS00.perm.bin"`

`PermToFBX.exe "Data\World\Game\**"`

## Wildcard Usage

A single `*` wildcard loads files only from the given directory. A double `**` wildcard performs a recursive search and loads files from all subdirectories as well.

## Command-line Options

<table>
  <thead>
    <tr>
      <th>Option</th>
      <th>Description</th>
      <th>Example</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>-output=&lt;path&gt; [optional]</code></td>
      <td>Path where the models & textures should be exported.</td>
      <td><code>-output=some_folder</code></td>
    </tr>
    <tr>
      <td><code>-rig=&lt;name&gt; [optional]</code></td>
      <td>Uses specific rig while exporting models.</td>
      <td><code>-rig=BasicFemale</code></td>
    </tr>
    <tr>
      <td><code>-model=&lt;name&gt; [optional]</code></td>
      <td>Export only specific model with this name.</td>
      <td><code>-model=SANDRA_SKIN_BODY</code></td>
    </tr>
  </tbody>
</table>