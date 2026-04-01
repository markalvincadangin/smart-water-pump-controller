import os
import re

dir_path = 'firmware/platformio_smart_water_pump_controller/src'
replacements = 0

def process_file(filepath):
    global replacements
    with open(filepath, 'r') as f:
        content = f.read()

    # 1. Serial.println("[COMP] msg");
    def repl_println_comp(m):
        comp = m.group(1).upper()
        msg = m.group(2).replace(r'\n', '')
        level = 'LOG_LEVEL_INFO'
        if 'err' in msg.lower() or 'fail' in msg.lower() or 'timeout' in msg.lower(): level = 'LOG_LEVEL_ERROR'
        if 'warn' in msg.lower() or 'rejected' in msg.lower(): level = 'LOG_LEVEL_WARN'
        return f'LOG({level}, "{comp}", "{msg}");'
    
    # 2. Serial.printf("[COMP] msg", args);
    def repl_printf_comp(m):
        comp = m.group(2).upper()
        msg = m.group(3).replace(r'\n', '')
        args = m.group(4)
        level = 'LOG_LEVEL_INFO'
        if 'err' in msg.lower() or 'fail' in msg.lower() or 'timeout' in msg.lower(): level = 'LOG_LEVEL_ERROR'
        if 'warn' in msg.lower() or 'rejected' in msg.lower(): level = 'LOG_LEVEL_WARN'
        # Check if the prefix has \n. if yes, remove it as LOG handles it.
        return f'LOG({level}, "{comp}", "{msg}", {args});'

    # 3. Serial.println("msg"); (without component)
    def repl_println_sys(m):
        msg = m.group(1).replace(r'\n', '')
        # Ignore empty or just formatting strings like \n=====
        if '===' in msg or msg.strip() == '':
            return m.group(0)
        return f'LOG(LOG_LEVEL_INFO, "SYS", "{msg}");'

    # Regex patterns
    # Matches: Serial.println("[COMP] some message");
    # Allows optional \n at the start/end of the string literal
    p1 = re.compile(r'Serial\.println\(\s*"\\?n?\[([^\]]+)\]\s*(.*?)\\?n?"\s*\);')
    content, n1 = p1.subn(repl_println_comp, content)

    # Matches: Serial.printf("\n[COMP] some message\n", args...);
    p2 = re.compile(r'Serial\.printf\(\s*"(.*?)\\?n?\[([^\]]+)\]\s*(.*?)\\?n?"\s*,\s*(.*?)\);', re.DOTALL)
    content, n2 = p2.subn(repl_printf_comp, content)

    # Matches: Serial.println("some message");
    p3 = re.compile(r'Serial\.println\(\s*"\\?n?([^\[].*?)\\?n?"\s*\);')
    content, n3 = p3.subn(repl_println_sys, content)

    changes = n1 + n2 + n3
    if changes > 0:
        with open(filepath, 'w') as f:
            f.write(content)
        replacements += changes
        print(f"Updated {filepath}: {changes} replacements")

for root, _, files in os.walk(dir_path):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.ino') or f.endswith('.h'):
            process_file(os.path.join(root, f))

print(f"Total Master Replacements: {replacements}")

# Now for Sensor Node: Replace SENSOR_DBGF and SENSOR_DBGLN with LOG_SN
sn_dir_path = 'firmware/platformio_sensor_node/src'
sn_replacements = 0

def process_sn_file(filepath):
    global sn_replacements
    with open(filepath, 'r') as f:
        content = f.read()

    # Match: SENSOR_DBGLN("[SN][WARN] msg"); -> LOG_SN(LOG_LEVEL_WARN, "SN", "msg");
    # Match: SENSOR_DBGF("[SN] msg", args); -> LOG_SN(LOG_LEVEL_INFO, "SN", "msg", args);
    
    def repl_sn_printf(m):
        tag1 = m.group(1) # [SN]
        tag2 = m.group(2) # [WARN] optional
        msg = m.group(3).replace(r'\n', '')
        args = m.group(4)
        
        level = 'LOG_LEVEL_INFO'
        comp = 'SN'
        if tag2 and 'warn' in tag2.lower(): level = 'LOG_LEVEL_WARN'
        if tag2 and 'err' in tag2.lower(): level = 'LOG_LEVEL_ERROR'
        if 'err' in msg.lower() or 'fail' in msg.lower(): level = 'LOG_LEVEL_ERROR'
        
        return f'LOG_SN({level}, "{comp}", "{msg}", {args});'

    def repl_sn_println(m):
        tag1 = m.group(1) # [SN]
        tag2 = m.group(2) # [WARN] optional
        msg = m.group(3).replace(r'\n', '')
        
        level = 'LOG_LEVEL_INFO'
        comp = 'SN'
        if tag2 and 'warn' in tag2.lower(): level = 'LOG_LEVEL_WARN'
        if tag2 and 'err' in tag2.lower(): level = 'LOG_LEVEL_ERROR'
        if 'err' in msg.lower() or 'fail' in msg.lower(): level = 'LOG_LEVEL_ERROR'
        
        return f'LOG_SN({level}, "{comp}", "{msg}");'

    p_sn_f = re.compile(r'SENSOR_DBGF\(\s*"(?:\[([^\]]+)\])?(?:\[([^\]]+)\])?\s*(.*?)\\?n?"\s*,\s*(.*?)\);', re.DOTALL)
    content, n1 = p_sn_f.subn(repl_sn_printf, content)

    p_sn_ln = re.compile(r'SENSOR_DBGLN\(\s*"(?:\[([^\]]+)\])?(?:\[([^\]]+)\])?\s*(.*?)\\?n?"\s*\);')
    content, n2 = p_sn_ln.subn(repl_sn_println, content)

    changes = n1 + n2
    if changes > 0:
        with open(filepath, 'w') as f:
            f.write(content)
        sn_replacements += changes
        print(f"Updated {filepath}: {changes} SN replacements")

for root, _, files in os.walk(sn_dir_path):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.ino') or f.endswith('.h'):
            process_sn_file(os.path.join(root, f))

print(f"Total SN Replacements: {sn_replacements}")

