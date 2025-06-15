import { Base64 } from 'js-base64';

export function encode(str) {
  try {
    return Base64.encode(str);
  } catch (e) {
    console.error("Error encoding string:", e);
    return ''; 
  }
}

export function decode(b64) {
  try {
    if (typeof b64 !== 'string') {
      console.warn("Attempted to decode non-string value:", b64);
      return '';
    }
    if (!/^[A-Za-z0-9+/=]*$/.test(b64)) {
       console.warn("Invalid Base64 characters detected:", b64);
    }
    let paddedB64 = b64;
    while (paddedB64.length % 4 !== 0) {
        paddedB64 += '=';
    }

    return Base64.decode(paddedB64);
  } catch (e) {
    console.error("Error decoding base64 string:", b64, e);
    return '';
  }
}