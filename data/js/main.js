(function () {
    "use strict";

    /*==================================================================
    [ Focus input ]*/
    document.querySelectorAll('.input100').forEach(function(input) {
        input.addEventListener('blur', function() {
            if(input.value.trim() !== "") {
                input.classList.add('has-val');
            }
            else {
                input.classList.remove('has-val');
            }
        });
    });

    /*==================================================================
    [ Validate ]*/
    var inputs = document.querySelectorAll('.validate-input .input100');
    
    document.querySelector('.validate-form').addEventListener('submit', function(event) {
        var check = true;

        inputs.forEach(function(input) {
            if (!validate(input)) {
                showValidate(input);
                check = false;
            }
        });

        if (!check) {
            event.preventDefault();
        }
    });

    document.querySelectorAll('.validate-form .input100').forEach(function(input) {
        input.addEventListener('focus', function() {
            hideValidate(input);
        });
    });

    function validate(input) {
        var type = input.getAttribute('type');
        var name = input.getAttribute('name');
        if (type === 'email' || name === 'email') {
            if (!input.value.trim().match(/^([a-zA-Z0-9_\-\.]+)@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.)|(([a-zA-Z0-9\-]+\.)+))([a-zA-Z]{1,5}|[0-9]{1,3})(\]?)$/)) {
                return false;
            }
        }
        else {
            if (input.value.trim() === '') {
                return false;
            }
        }
        return true;
    }

    function showValidate(input) {
        var parent = input.parentElement;
        parent.classList.add('alert-validate');
    }

    function hideValidate(input) {
        var parent = input.parentElement;
        parent.classList.remove('alert-validate');
    }

    /*==================================================================
    [ Show pass ]*/
    var showPass = 0;
    document.querySelectorAll('.btn-show-pass').forEach(function(button) {
        button.addEventListener('click', function() {
            var input = button.nextElementSibling;
            if (showPass === 0) {
                input.setAttribute('type', 'text');
                button.querySelector('i').classList.remove('zmdi-eye');
                button.querySelector('i').classList.add('zmdi-eye-off');
                showPass = 1;
            }
            else {
                input.setAttribute('type', 'password');
                button.querySelector('i').classList.add('zmdi-eye');
                button.querySelector('i').classList.remove('zmdi-eye-off');
                showPass = 0;
            }
        });
    });
})();

function updateSsid() {
    // select 요소에서 선택된 값을 가져와 ssid input에 설정
    var selectedSsid = document.getElementById('list').value;
    document.getElementById('ssid').value = selectedSsid;
  }
