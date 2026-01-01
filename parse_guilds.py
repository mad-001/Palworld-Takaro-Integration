#!/usr/bin/env python3
"""
Palworld Guild Parser - Extracts guild data from Level.sav files
Usage: python3 parse_guilds.py <path_to_Level.sav>
Output: Simple text format with guild names and members
"""
import sys
import os

# Add palworld-save-tools to path
sys.path.insert(0, '/tmp/palworld-save-tools')

from palworld_save_tools.palsav import decompress_sav_to_gvas
from palworld_save_tools.gvas import GvasFile
from palworld_save_tools.paltypes import PALWORLD_TYPE_HINTS, PALWORLD_CUSTOM_PROPERTIES

def parse_guilds(save_path):
    """Parse guilds from a Palworld save file"""
    try:
        # Suppress library output during parsing
        import io
        import contextlib

        # Load save file
        with open(save_path, 'rb') as f:
            data = f.read()

        # Decompress and parse with suppressed output
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            raw_gvas, _ = decompress_sav_to_gvas(data)
            gvas = GvasFile.read(raw_gvas, PALWORLD_TYPE_HINTS, PALWORLD_CUSTOM_PROPERTIES)

        world_data = gvas.properties['worldSaveData']['value']

        # Extract guilds from GroupSaveDataMap
        groups = world_data.get('GroupSaveDataMap', {}).get('value', [])
        guilds = []

        for group in groups:
            try:
                group_type_value = group['value']['GroupType']['value']['value']

                # Only process guilds (not organizations)
                if group_type_value == 'EPalGroupType::Guild':
                    raw_data = group['value']['RawData']['value']

                    guild_id = raw_data.get('group_id', '')
                    guild_name = raw_data.get('guild_name', 'Unknown Guild')
                    if not guild_name or guild_name.strip() == '':
                        guild_name = 'Unnamed Guild'

                    # Get player names directly from the 'players' field
                    players = raw_data.get('players', [])
                    member_names = []
                    for player in players:
                        player_info = player.get('player_info', {})
                        player_name = player_info.get('player_name', '')
                        if player_name:
                            member_names.append(player_name)

                    if member_names:  # Only include guilds with known players
                        guilds.append({
                            'name': guild_name,
                            'members': member_names
                        })
            except:
                pass

        return guilds

    except Exception as e:
        print(f"ERROR: {str(e)}", file=sys.stderr)
        return []

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python3 parse_guilds.py <path_to_Level.sav>", file=sys.stderr)
        sys.exit(1)

    save_path = sys.argv[1]

    if not os.path.exists(save_path):
        print(f"ERROR: Save file not found: {save_path}", file=sys.stderr)
        sys.exit(1)

    guilds = parse_guilds(save_path)

    # Output guilds in simple format (suppress warnings to stderr)
    for guild in guilds:
        print(f"Guild: {guild['name']}")
        print(f"Members: {len(guild['members'])}")
        for member in guild['members']:
            print(f"  - {member}")
