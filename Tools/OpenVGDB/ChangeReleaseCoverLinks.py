"""
Change the domain of the links of the images in OpenVGDB
"""

import os.path
import sqlite3

# Variables
database = "openvgdb.sqlite"                    # Database file

currentDomain = 'http://img.gamefaqs.net/'      # Currently used domain
newDomain = 'http://gamefaqs1.cbsistatic.com/'  # New domain

# Open connection
connection = sqlite3.connect(database)

# Read all relevant rows
cursor = connection.cursor()
cursor.execute('SELECT romId,releaseCoverFront,releaseCoverBack FROM RELEASES WHERE (releaseCoverFront IS NOT NULL) OR (releaseCoverBack IS NOT NULL)')
selection = cursor.fetchall()

# Iterate through the found rows
query = ''
for row in selection:
    # row[0] = romId
    # row[1] = releaseCoverFront
    # row[2] = releaseCoverBack
    releaseCoverFront = row[1].replace(currentDomain, newDomain)

    if row[2]:
        # row contains releaseCoverBack
        releaseCoverBack = row[2].replace(currentDomain, newDomain)
        connection.execute(f"UPDATE RELEASES SET releaseCoverFront = '{releaseCoverFront}', releaseCoverBack = '{releaseCoverBack}' WHERE romId = {row[0]}")
        print(f'ROM {row[0]}: {row[1]} -> {releaseCoverFront}, {row[2]} -> {releaseCoverBack}')

    else:
        # row doesn't contain releaseCoverBack
        connection.execute(f"UPDATE RELEASES SET releaseCoverFront = '{releaseCoverFront}' WHERE romId = {row[0]}")
        print(f'ROM {row[0]}: {row[1]} -> {releaseCoverFront}')

# Commit and close
connection.commit()
connection.close()