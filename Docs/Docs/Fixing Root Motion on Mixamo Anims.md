1. Install Blender if it isn't already
2. Install [Mixamo Rootbaker addon](https://github.com/enziop/mixamo_converter)
3. Download some Mixamo animations
4. Make sure only animations exist in the folder you plan to convert (No XBot)
5. These settings seem to work best
	- Make the output folder somewhere UE is not aware of so it won't auto-import
![[Pasted image 20251116124129.png]]
6. After converting, import a single converted animation as is (we want its skeleton)
7. Now import the rest, and specify the skeleton created by the first one
8. Retarget to your desired skeleton
9. In the resulting animations, set "Enable Root Motion" to true