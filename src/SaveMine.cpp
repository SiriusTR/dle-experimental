// Copyright (c) 1998 Bryan Aamot, Brainware
#include "mine.h"
#include "dle-xp.h"

#include "rapidjson\document.h"
#include "rapidjson\filewritestream.h"
#include "rapidjson\pointer.h"
#include "rapidjson\prettywriter.h"

// -----------------------------------------------------------------------------

short CMine::Save (const char * szFile)
{
if (segmentManager.Overflow ()) {
	if (vertexManager.Overflow ()) 
		ErrorMsg ("Error: Too many segments and vertices for this level version!\nThe level cannot be saved.");
	else
		ErrorMsg ("Error: Too many segments for this level version!\nThe level cannot be saved.");
	return 0;
	}
else if (vertexManager.Overflow ()) {
	ErrorMsg ("Error: Too many vertices for this level version!\nThe level cannot be saved.");
	return 0;
	}

	CFileManager	fp;
	char				filename [256];
	int				mineDataOffset, gameDataOffset, hostageTextOffset;
	int				mineErr, gameErr;

UpdateCenter ();
strcpy_s (filename, sizeof (filename), szFile);
if (!fp.Open (filename, "w+b"))
	return 0;

m_changesMade = 0;
// write file signature
fp.WriteInt32 ('P'*0x1000000L + 'L'*0x10000L + 'V'*0x100 + 'L'); // signature

// always save as version 7 or greater if its a D2 level
// otherwise, blinking lights will not work.
if (LevelVersion () < 7 && IsD2File ()) {
	SetLevelVersion (7);
	}
if (IsD2XLevel () && LevelIsOutdated ()) {
	UpdateLevelVersion ();
	segmentManager.UpdateWalls (MAX_WALLS_D2 + 1, WALL_LIMIT);
	}

// write version
fp.WriteInt32 (LevelVersion ());
fp.WriteInt32 (0); // mineDataOffset (temporary)
fp.WriteInt32 (0); // gameDataOffset (temporary)

if (IsD1File ())
	fp.WriteInt32 (0); // hostageTextOffset (temporary)
else if (IsD2File ()) {
	if (LevelVersion () >= 8) {
		fp.WriteInt16 (rand ());
		fp.WriteInt16 (rand ());
		fp.WriteInt16 (rand ());
		fp.WriteSByte ((sbyte) rand ());
		}
	// save palette name
	char *name = strrchr (descentFolder [1], '\\');
	if (!name) 
		name = descentFolder [1]; // point to 1st char if no slash found
	else
		name++;               // point to character after slash
	char paletteName [15];
	strncpy_s (paletteName, sizeof (paletteName), name, 12);
	paletteName [13] = null;  // null terminate just in case
	// replace extension with *.256
	if (strlen (paletteName) > 4)
		strcpy_s (&paletteName [strlen (paletteName) - 4], 5, ".256");
	else
		strcpy_s (paletteName, sizeof (paletteName), "groupa.256");
	//_strupr_s (paletteName, sizeof (paletteName));
	strcat_s (paletteName, sizeof (paletteName), "\n"); // add a return to the end
	fp.Write (paletteName, (int) strlen (paletteName), 1);
	// write reactor info
	fp.Write (ReactorTime ());
	fp.Write (ReactorStrength ());
	// variable light new for version 7
	lightManager.WriteVariableLights (&fp);
	// write secret segment number
	fp.Write (SecretSegment ());
	// write secret return segment orientation
	fp.Write (SecretOrient ());
	}
// save mine data
mineDataOffset = fp.Tell ();
if (0 > (mineErr = SaveMineGeometry (&fp))) {
	fp.Close ();
	ErrorMsg ("Error saving mine data");
	return -2;
	}

// save game data
gameDataOffset = fp.Tell ();
if (0 > (gameErr = SaveGameItems (&fp))) {
	fp.Close ();
	ErrorMsg ("Error saving game data");
	return -3;
	}

// save hostage data
hostageTextOffset = fp.Tell ();
// leave hostage text empty

// now and go back to beginning of file and save offsets
fp.Seek (2 * sizeof (int), SEEK_SET);
fp.Write (mineDataOffset);    // mineDataOffset
fp.Write (gameDataOffset);    // gameDataOffset
if (m_fileType == RDL_FILE) 
	fp.Write (hostageTextOffset); // hostageTextOffset
fp.Seek (0, SEEK_END);
fp.Close ();
return 1;
}

// ------------------------------------------------------------------------
// SaveMineGeometry()
//
// ACTION - Writes a mine data portion of RDL file.
// ------------------------------------------------------------------------

short CMine::SaveMineGeometry (CFileManager* fp)
{
// write version (1 ubyte)
fp->WriteByte (COMPILED_MINE_VERSION);
// write no. of vertices (2 bytes)
fp->WriteUInt16 (vertexManager.Count ());
// write number of Segments () (2 bytes)
fp->WriteInt16 (segmentManager.Count ());
// write all vertices
vertexManager.SetIndex ();
segmentManager.SetIndex ();
triggerManager.SetIndex ();
wallManager.SetIndex ();

vertexManager.Write (fp);
// write segment information
segmentManager.WriteSegments (fp);
// for Descent 2, save special info here
if (LevelVersion () >= 9)
	lightManager.WriteColors (*fp);
return 1;
}

// ------------------------------------------------------------------------
// SaveGameItems()
//
//  ACTION - Saves the player, object, wall, door, trigger, and
//           material generator data from an RDL file.
// ------------------------------------------------------------------------

short CMine::SaveGameItems (CFileManager* fp)
{
if (IsD1File ()) {
	Info ().fileInfo.signature = 0x6705;
	Info ().fileInfo.version = 25;
	Info ().fileInfo.size = 119;
	Info ().level = 0;
	}
else {
	Info ().fileInfo.signature = 0x6705;
	Info ().fileInfo.version = (LevelVersion () < 13) ? 31 : 40;
	Info ().fileInfo.size = (LevelVersion () < 13) ? 143 : -1; // same as sizeof (Info ())
	Info ().level = 0;
	}

int startOffset = fp->Tell ();
Info ().Write (fp, IsD2XLevel ());
if (Info ().fileInfo.version >= 14) // save mine file name
	fp->Write (m_currentLevelName, sizeof (char), (int) strlen (m_currentLevelName));
if (IsD2File ())
	fp->Write ("\n", 1, 1); // write an end - of - line
else
	fp->Write ("", 1, 1);   // write a null

// write pof names from resource file
short	nSavePofNames;

if (IsD2File ()) {
	nSavePofNames = 166;
	fp->WriteUInt16 (166);   // write # of POF names
	}
else {
	nSavePofNames = 78;
	fp->WriteInt16 (25);	// Don't know exactly what this value is for or why it is 25?
	}

CResource res;
ubyte* savePofNames = res.Load (IsD1File () ? IDR_POF_NAMES1 : IDR_POF_NAMES2);
if (savePofNames == null)
	return 0;
fp->Write (savePofNames, nSavePofNames, 13); // 13 characters each

Info ().player.offset = fp->Tell ();
char* str = "Made with Descent Level Editor\0\0\0\0\0\0\0";
fp->Write (str, (int) strlen (str) + 1, 1);

segmentManager.RenumberProducers ();
objectManager.Write (fp);
wallManager.Write (fp);
triggerManager.Write (fp);
triggerManager.WriteReactor (fp);
segmentManager.WriteRobotMakers (fp);
if (IsD2File ()) {
	segmentManager.WriteEquipMakers (fp);
	lightManager.WriteLightDeltas (fp);
	}

fp->Seek (startOffset, SEEK_SET);
Info ().Write (fp, IsD2XLevel ());
fp->Seek (0, SEEK_END);
return 1;
}

// -----------------------------------------------------------------------------

using namespace rapidjson;

short CMine::ExportOverload (const char * filename)
{
	CFileManager fp;

if (!fp.Open (filename, "w+b"))
	return 0;
else {
	Document document;
	document.SetObject ();
	auto& allocator = document.GetAllocator ();
	char path [128] = { 0 };

	document.AddMember ("properties", Value ().SetObject (), allocator);
	document.AddMember ("global_data", Value ().SetObject (), allocator);
	document.AddMember ("challenge_data_string", "", allocator);
	document.AddMember ("verts", Value ().SetObject (), allocator);
	document.AddMember ("segments", Value ().SetObject (), allocator);
	document.AddMember ("entities", Value ().SetObject (), allocator);

	// Level properties
	Pointer ("/properties/next_segment").Set (document, segmentManager.Count ());
	Pointer ("/properties/next_vertex").Set (document, vertexManager.Count ());
	Pointer ("/properties/next_entity").Set (document, 0);
	Pointer ("/properties/selected_segment").Set (document, 0);
	Pointer ("/properties/selected_side").Set (document, 1);
	Pointer ("/properties/selected_vertex").Set (document, 0);
	Pointer ("/properties/selected_entity").Set (document, -1);
	Pointer ("/properties/num_segments").Set (document, segmentManager.Count ());
	Pointer ("/properties/num_vertices").Set (document, vertexManager.Count ());
	Pointer ("/properties/num_entities").Set (document, 0);
	Pointer ("/properties/num_marked_segments").Set (document, 0);
	Pointer ("/properties/num_marked_sides").Set (document, 0);
	Pointer ("/properties/num_marked_vertices").Set (document, 0);
	Pointer ("/properties/num_marked_entities").Set (document, 0);
	Pointer ("/properties/texture_set").Set (document, "Titan - Bronze");

	// Global data - just use default settings
	Pointer ("/global_data/grid_size").Set (document, 8);
	Pointer ("/global_data/pre_smooth").Set (document, 3);
	Pointer ("/global_data/post_smooth").Set (document, 0);
	Pointer ("/global_data/simplify_strength").Set (document, 0.0);
	Pointer ("/global_data/deform_presets0").Set (document, "PLAIN_NOISE");
	Pointer ("/global_data/deform_presets1").Set (document, "NONE");
	Pointer ("/global_data/deform_presets2").Set (document, "NONE");
	Pointer ("/global_data/deform_presets3").Set (document, "NONE");

	// Vertices
	for (int nVertex = 0; nVertex < vertexManager.Count (); nVertex++) {
		CVertex& vertex = vertexManager [nVertex];
		sprintf_s (path, "/verts/%d/marked", nVertex);
		Pointer (path).Set (document, false);
		sprintf_s (path, "/verts/%d/x", nVertex);
		Pointer (path).Set (document, vertex.v.x / 5);
		sprintf_s (path, "/verts/%d/y", nVertex);
		Pointer (path).Set (document, vertex.v.y / 5);
		sprintf_s (path, "/verts/%d/z", nVertex);
		Pointer (path).Set (document, vertex.v.z / 5);
		}

	// Segments
	for (int nSegment = 0; nSegment < segmentManager.Count (); nSegment++) {
		CSegment* pSegment = segmentManager.Segment (nSegment);
		sprintf_s (path, "/segments/%d/marked", nSegment);
		Pointer (path).Set (document, false);
		sprintf_s (path, "/segments/%d/pathfinding", nSegment);
		Pointer (path).Set (document, "All");
		sprintf_s (path, "/segments/%d/exitsegment", nSegment);
		Pointer (path).Set (document, "None");
		sprintf_s (path, "/segments/%d/dark", nSegment);
		Pointer (path).Set (document, false);

		// Vertex IDs
		sprintf_s (path, "/segments/%d/verts", nSegment);
		Value& verts = Pointer (path).Create (document).SetArray ();
		for (int nVert = 0; nVert < MAX_VERTICES_PER_SEGMENT; nVert++)
			verts.PushBack (pSegment->VertexId (nVert), allocator);

		// Sides
		sprintf_s (path, "/segments/%d/sides", nSegment);
		Value& sides = Pointer (path).Create (document).SetArray ();
		for (int nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
			CSide *pSide = pSegment->Side (nSide);
			Value side (kObjectType);
			Pointer ("/marked").Set (side, false, allocator);
			Pointer ("/chunk_plane_order").Set (side, -1, allocator);
			Pointer ("/tex_name").Set (side, "concrete_test", allocator);
			Pointer ("/deformation_preset").Set (side, 0, allocator);
			Pointer ("/deformation_height").Set (side, 0.0, allocator);

			Value& sideVerts = Pointer ("/verts").Create (side, allocator).SetArray ();
			Value& sideUVs = Pointer ("/uvs").Create (side, allocator).SetArray ();
			for (int nSideVert = 0; nSideVert < pSide->VertexCount (); nSideVert++) {
				sideVerts.PushBack (pSegment->VertexId (nSide, nSideVert), allocator);
				Value uv (kObjectType);
				uv.AddMember ("u", pSide->Uvls (nSideVert)->u, allocator);
				uv.AddMember ("v", pSide->Uvls (nSideVert)->v, allocator);
				sideUVs.PushBack (uv, allocator);
				}

			Value& decals = Pointer ("/decals").Create (side, allocator).SetArray ();
			Value decal (kObjectType);
			Pointer ("/mesh_name").Set (decal, "", allocator);
			Pointer ("/align").Set (decal, "CENTER", allocator);
			Pointer ("/mirror").Set (decal, "OFF", allocator);
			Pointer ("/rotation").Set (decal, 0, allocator);
			Pointer ("/repeat_u").Set (decal, 1, allocator);
			Pointer ("/repeat_v").Set (decal, 1, allocator);
			Pointer ("/offset_u").Set (decal, 0, allocator);
			Pointer ("/offset_v").Set (decal, 0, allocator);
			Pointer ("/hidden").Set (decal, false, allocator);
			Value& clips = Pointer ("/clips").Create (decal, allocator).SetArray ();
			clips.PushBack ("NONE", allocator);
			clips.PushBack ("NONE", allocator);
			clips.PushBack ("NONE", allocator);
			clips.PushBack ("NONE", allocator);
			Value& caps = Pointer ("/caps").Create (decal, allocator).SetArray ();
			caps.PushBack ("NONE", allocator);
			caps.PushBack ("NONE", allocator);
			caps.PushBack ("NONE", allocator);
			caps.PushBack ("NONE", allocator);
			// Two decal slots per side
			decals.PushBack (Value ().CopyFrom (decal, allocator), allocator);
			decals.PushBack (decal, allocator);

			Pointer ("/door").Set (side, -1, allocator);
			sides.PushBack (side, allocator);
			}

		// Neighboring segments
		sprintf_s (path, "/segments/%d/neighbors", nSegment);
		Value& neighbors = Pointer (path).Create (document).SetArray ();
		for (int nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++)
			neighbors.PushBack (pSegment->ChildId (nSide), allocator);
		}

	char writeBuffer [65536] = { 0 };
	FileWriteStream stream (fp.File (), writeBuffer, sizeof (writeBuffer));
	PrettyWriter <FileWriteStream> writer (stream);
	writer.SetIndent (' ', 2);
	document.Accept (writer);
	}

fp.Close ();
return 1;
}

// --------------------------------------------------------------------------
//eof mine.cpp