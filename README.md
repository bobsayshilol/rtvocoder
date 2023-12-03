# Realtime vocoder

This project was a joke to see if I could vocode myself during a discord call.

It's overdesigned with the intention of being able to control the plugin via a secondary tool, but that wasn't needed in the end.

A LADSPA plugin is available when building this repo, though it's untested.

## Structure

#### cmdline
- Offline tool to run and benchmark the filters.
#### lib
- The guts of the filters.
#### libs (terrible name)
- Third party libs (LADSPA and LV2 headers).
#### module
- The LADSPA/LV2 plugins. These are thin wrappers around the plugins in 'internal' and handle the IPC coms.
#### module/internal
- The hotswappable filters that the plugin would reload/modify when asked to.
#### pipewire
- Tool to load the plugin and control it (though the latter was never implemented).
#### tests
- Tests.

## References

* Audacity's vocoder implementation
* https://docs.pipewire.org/page_api.html
* https://github.com/noisetorch/NoiseTorch
